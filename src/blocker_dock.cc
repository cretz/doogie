#include "blocker_dock.h"

#include <iterator>
#include <set>

#include "main_window.h"
#include "profile.h"

namespace doogie {

BlockerDock::BlockerDock(const Cef& cef,
                         BrowserStack* browser_stack,
                         QWidget* parent)
    : QDockWidget("Request Blocker", parent),
      cef_(cef) {
  setFeatures(QDockWidget::AllDockWidgetFeatures);

  auto layout = new QVBoxLayout;

  auto top_layout = new QHBoxLayout;
  top_layout->addWidget(
      new QLabel(QString("Blocked Requests (max %1):").
                 arg(kMaxTableCount)), 1);

  current_only_ = new QCheckBox("Current Page");
  current_only_->setToolTip("Only show requests for current page");
  top_layout->addWidget(current_only_);

  auto clear_button = new QToolButton;
  clear_button->setToolTip("Clear Requests");
  clear_button->setIcon(QIcon(":/res/images/fontawesome/ban.png"));
  clear_button->setAutoRaise(true);
  top_layout->addWidget(clear_button);

  auto settings_button = new QToolButton;
  settings_button->setToolTip("Profile Blocker Settings");
  settings_button->setIcon(QIcon(":/res/images/fontawesome/cogs.png"));
  settings_button->setAutoRaise(true);
  top_layout->addWidget(settings_button);

  layout->addLayout(top_layout);

  table_ = new QTableWidget;
  table_->setColumnCount(8);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setHorizontalHeaderLabels(
      { "Target Domain", "Target URL", "Referrer Domain", "Referrer URL",
        "Rule List", "Line Number", "Rule", "Time" });
  table_->setTextElideMode(Qt::ElideRight);
  table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
  table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
  table_->horizontalHeader()->setSectionResizeMode(
        5, QHeaderView::ResizeToContents);
  table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
  table_->horizontalHeader()->setSectionResizeMode(
        7, QHeaderView::ResizeToContents);
  table_->verticalHeader()->setVisible(false);
  table_->sortByColumn(7, Qt::DescendingOrder);
  layout->addWidget(table_, 1);

  auto widg = new QWidget;
  widg->setLayout(layout);
  setWidget(widg);

  auto remove_browser_requests = [=](BrowserWidget* widg) {
    // Remove the browser-specific part
    blocked_requests_by_browser_.remove(widg);
    // Remove the general ones (backwards to save indices)
    for (int i = blocked_requests_.size() - 1; i >= 0; i--) {
      if (blocked_requests_[i].browser == widg) blocked_requests_.remove(i);
    }
  };

  // Rebuild on current change
  connect(current_only_, &QCheckBox::toggled, [=](bool) { RebuildTable(); });
  // Clear on clear press
  connect(clear_button, &QToolButton::clicked, [=](bool) {
    // If it's current only, just get rid of the current
    if (current_only_->isChecked()) {
      remove_browser_requests(current_browser_);
    } else {
      // Get rid of everything everywhere
      blocked_requests_by_browser_.clear();
      blocked_requests_.clear();
    }
    RebuildTable();
  });
  // Show the settings dialog
  connect(settings_button, &QToolButton::clicked, [=](bool) {
    MainWindow::EditProfileSettings([=](ProfileSettingsDialog* dialog) {
      dialog->SetCurrentTab(3);
    });
  });
  // Remove the refs when destroyed
  connect(browser_stack, &BrowserStack::BrowserDestroyed,
          [=](BrowserWidget* widg) {
    remove_browser_requests(widg);
    // Rebuild if we're generic
    if (!current_only_->isChecked()) RebuildTable();
  });
  // When current changes and the current is set, we need to rebuild
  connect(browser_stack, &BrowserStack::BrowserChanged,
          [=](BrowserWidget* widg) {
    current_browser_ = widg;
    if (current_only_->isChecked()) RebuildTable();
  });

  // Set callback to this to check the rule
  browser_stack->SetResourceLoadCallback(
        [=](BrowserWidget* browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefRequest> request) -> bool {
    // If the URL is an abp:subscribe URL, add it
    auto url_str = QString::fromStdString(request->GetURL().ToString());
    if (url_str.startsWith("abp:subscribe")) {
      QUrl url(url_str);
      QUrlQuery url_query(url);
      auto location = url_query.queryItemValue("location", QUrl::FullyDecoded);
      if (!location.isEmpty()) {
        // Defer the subscription so we can cancel the load right now
        Util::RunOnMainThread([=]() {
          QTimer::singleShot(0, [=]() { SubscribeRuleList(location); });
        });
        return false;
      }
    }
    return IsAllowedToLoad(browser, frame, request);
  });

  // Do a local-file-only load to start with. Then we'll check
  // for updates a few mins later.
  ProfileUpdated(true);
  // Check for updates after the normal timeout plus 10 seconds
  QTimer::singleShot((kListLoadTimeoutSeconds + 10) * 1000,
                     [=]() { CheckUpdate(); });
  // And every so often
  startTimer(kCheckUpdatesSeconds * 1000);
}

BlockerDock::~BlockerDock() {
  QMutexLocker locker(&rules_mutex_);
  if (rules_) {
    delete rules_;
    rules_ = nullptr;
  }
}

void BlockerDock::ProfileUpdated(bool load_local_file_only) {
  // As a shortcut, if there are no enabled blockers, just null out the
  //  rule set.
  if (Profile::Current().EnabledBlockerListIds().isEmpty()) {
    next_rule_set_unique_num_++;
    lists_.clear();
    QMutexLocker locker(&rules_mutex_);
    if (rules_) delete rules_;
    rules_ = nullptr;
    return;
  }

  // We have to be thread safe here. So what we will do is load the list
  //  info here and store in a bunch of local vars and work on those until
  //  the end. We have to make sure we are on the same thread doing work
  //  w/ local members, so we run some things on the main thread. We also
  //  need a timeout.
  // This unique number is how we know at the end we're still around.
  next_rule_set_unique_num_++;
  auto my_rule_set_num = next_rule_set_unique_num_;

  // Load up the list into the hash w/ file index. Deleted at the end
  auto new_lists = new QHash<int, BlockerList>();
  auto db_lists =
      BlockerList::Lists(Profile::Current().EnabledBlockerListIds());
  for (auto& list : db_lists) {
    // Try to use the existing index for existing list
    auto new_index = -1;
    for (auto key : lists_.keys()) {
      if (lists_[key].Id() == list.Id()) {
        new_index = key;
        break;
      }
    }
    // Need a new index that doesn't clash w/ current
    if (new_index == -1) {
      new_index = new_lists->size() + lists_.size();
      for (; lists_.contains(new_index); new_index++) { }
    }
    new_lists->insert(new_index, list);
  }

  // The rule list we will operate on
  auto new_rules = new BlockerRules;
  // The cancellation list, by list ID (not file index). This is
  //  delete in the full load complete callback.
  auto cancellations = new QHash<qlonglong, std::function<void()>>();

  // The timeout timer that will be started at the end. We use this
  // object's presence to determine if we're done.
  QPointer<QTimer> timeout_timer = new QTimer;

  // Callback for when we're all done. Should only be called on one thread.
  auto full_load_complete = [=]() {
    // We use the timer to determine reentrancy
    if (!timeout_timer) return;
    // Delete the timeout timer
    delete timeout_timer.data();

    // Cancel everything as necessary
    for (auto& to_cancel : cancellations->values()) to_cancel();
    delete cancellations;

    if (my_rule_set_num == next_rule_set_unique_num_) {
      next_rule_set_unique_num_++;
      // We're ok w/ the non-atomicness of the lists here because
      //  we only look it up on GUI threads usually.
      lists_ = *new_lists;
      // Go over each list and reload it
      for (auto index : lists_.keys()) lists_[index].Reload();
      // Set new rules
      QMutexLocker locker(&rules_mutex_);
      if (rules_) delete rules_;
      rules_ = new_rules;
    }
    delete new_lists;
  };

  // Go over each and start the list load. Note, we choose to loop over
  //  the keys so we can get the ref of the list so it updates things like
  //  last refreshed.
  for (auto list_index : new_lists->keys()) {
    auto& list = (*new_lists)[list_index];
    cancellations->insert(list_index, list.LoadRules(
          cef_, list_index, load_local_file_only,
          [=](QList<BlockerRules::Rule*> rules, bool ok) {
      // Get back on proper thread in event loop
      Util::RunOnMainThread([=]() {
        // Timer will be gone if timed out
        if (timeout_timer) {
          if (ok) {
            new_rules->AddRules(rules);
            qDeleteAll(rules);
          }
          cancellations->remove(list_index);
          if (cancellations->isEmpty()) full_load_complete();
        }
      });
    }));
  }

  // Start the timeout timer
  timeout_timer->setSingleShot(true);
  connect(timeout_timer, &QTimer::timeout, full_load_complete);
  timeout_timer->start(kListLoadTimeoutSeconds * 1000);
}

void BlockerDock::timerEvent(QTimerEvent*) {
  CheckUpdate();
}

void BlockerDock::CheckUpdate() {
  for (auto list : lists_.values()) {
    if (list.NeedsUpdate()) {
      ProfileUpdated();
      return;
    }
  }
}

bool BlockerDock::IsAllowedToLoad(BrowserWidget* browser,
                                  CefRefPtr<CefFrame>,
                                  CefRefPtr<CefRequest> request) {
  QMutexLocker locker(&rules_mutex_);
  if (!rules_) return true;
  QElapsedTimer timer;
  timer.start();
  auto target_url_str = QString::fromStdString(request->GetURL().ToString());
  QUrl target_url(target_url_str, QUrl::StrictMode);
  // Ref is same as target if not present
  auto ref_url_str = QString::fromStdString(
        request->GetReferrerURL().ToString());
  auto ref_url = ref_url_str.isEmpty() ? target_url :
                                         QUrl(ref_url_str, QUrl::StrictMode);
  auto type = TypeFromRequest(target_url, request);
  auto result = rules_->FindStaticRule(target_url, ref_url, type);
  if (!result) return true;

  // Add a few more details before deleting the result
  auto req_file_index = result->info.file_index;
  auto req_line_number = result->info.line_num;
  auto req_rule = result->ToRuleString();
  auto req_time = QDateTime::currentDateTime();
  delete result;
  result = nullptr;

  // We choose to defer the rest of this to the event loop to get to
  //  the common thread.
  Util::RunOnMainThread([=]() {
    BlockedRequest req = {};
    req.browser = browser;
    req.target_url = target_url;
    req.ref_url = ref_url;
    // Add the list name here to make sure lists_ is good.
    if (lists_.contains(req_file_index)) {
      req.rule_list = lists_[req_file_index].Name();
    }
    req.line_number = req_line_number;
    req.rule = req_rule;
    qDebug() << "Blocked " << req.target_url;
    req.time = req_time;

    // Now add it to the browser-specific and general vectors
    auto& by_browser = blocked_requests_by_browser_[browser];
    by_browser << req;
    if (by_browser.size() > kMaxTableCount) by_browser.removeFirst();
    blocked_requests_ << req;
    if (blocked_requests_.size() > kMaxTableCount) {
      blocked_requests_.removeFirst();
    }

    // Now add it to the table if it applies to us
    if (!current_only_->isChecked() || current_browser_ == req.browser) {
      bool was_sorted = table_->isSortingEnabled();
      table_->setSortingEnabled(false);
      // First, we need to remove the least recent if it will get oversized.
      //  The way we will do this is to capture the current sort, sort by the
      //  date column, take the first, the put the sort back.
      if (table_->rowCount() == kMaxTableCount) {
        auto prev_sort_col =
            table_->horizontalHeader()->sortIndicatorSection();
        auto prev_sort_ord = table_->horizontalHeader()->sortIndicatorOrder();
        table_->sortItems(7, Qt::DescendingOrder);
        table_->removeRow(table_->rowCount() - 1);
        // Only if it was sorted, if it wasn't, well it is now :-)
        if (was_sorted) table_->sortItems(prev_sort_col, prev_sort_ord);
      }
      AppendTableRow(req);
      table_->setSortingEnabled(true);
    }
  });

  return false;
}

void BlockerDock::RebuildTable() {
  table_->setSortingEnabled(false);
  table_->setRowCount(0);

  const QVector<BlockedRequest>& to_render = current_only_->isChecked() ?
        blocked_requests_by_browser_.value(current_browser_) :
        blocked_requests_;

  for (const auto& req : to_render) AppendTableRow(req);

  table_->setSortingEnabled(true);
}

void BlockerDock::AppendTableRow(const BlockedRequest& request) {
  auto row = table_->rowCount();
  table_->setRowCount(row + 1);
  auto str_item = [](const QString& s) {
    auto item = new QTableWidgetItem(s);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    item->setToolTip(s);
    return item;
  };
  auto long_item = [](const QString& s, qlonglong i) {
    auto item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    item->setData(Qt::EditRole, i);
    item->setData(Qt::DisplayRole, s);
    item->setToolTip(s);
    return item;
  };

  table_->setItem(row, 0, str_item(request.target_url.host()));
  table_->setItem(row, 1, str_item(request.target_url.toString()));
  table_->setItem(row, 2, str_item(request.ref_url.host()));
  table_->setItem(row, 3, str_item(request.ref_url.toString()));
  if (request.rule_list.isEmpty()) {
    table_->setItem(row, 4, str_item("<unknown>"));
    table_->setItem(row, 5, str_item(""));
  } else {
    table_->setItem(row, 4, str_item(request.rule_list));
    table_->setItem(row, 5, long_item(
        QString::number(request.line_number), request.line_number));
  }
  table_->setItem(row, 6, str_item(request.rule));
  table_->setItem(row, 7, long_item(request.time.toString(),
                                    request.time.toMSecsSinceEpoch()));
}

BlockerRules::StaticRule::RequestType BlockerDock::TypeFromRequest(
    const QUrl& target_url,
    CefRefPtr<CefRequest> request) {
  switch (request->GetResourceType()) {
    case RT_SCRIPT: return BlockerRules::StaticRule::Script;
    case RT_IMAGE: return BlockerRules::StaticRule::Image;
    case RT_STYLESHEET: return BlockerRules::StaticRule::Stylesheet;
    // Note, this means that there is no way to get ObjectSubRequest
    case RT_OBJECT: return BlockerRules::StaticRule::Object;
    case RT_XHR: return BlockerRules::StaticRule::XmlHttpRequest;
    case RT_SUB_FRAME: return BlockerRules::StaticRule::SubDocument;
    case RT_PING: return BlockerRules::StaticRule::Ping;
    case RT_FONT_RESOURCE: return BlockerRules::StaticRule::Font;
    case RT_MEDIA: return BlockerRules::StaticRule::Media;
    default:
      // no-op to satisfy exhaustion warnings
      break;
  }
  if (target_url.scheme() == "ws" || target_url.scheme() == "wss") {
    return BlockerRules::StaticRule::WebSocket;
  }
  // TODO(cretz): Need to support webrtc blocking
  // TODO(cretz): Need to support popup
  return BlockerRules::StaticRule::Other;
}

void BlockerDock::SubscribeRuleList(const QString& url) {
  // We show the settings dialog, and attempt to add the URL
  MainWindow::EditProfileSettings([=](ProfileSettingsDialog* dialog) {
    dialog->SetCurrentTab(3);
    dialog->TryAddBlockerListUrl(url);
  });
}

}  // namespace doogie
