#include "blocker_dock.h"

#include "profile.h"

namespace doogie {

BlockerDock::BlockerDock(const Cef& cef,
                         BrowserStack* browser_stack,
                         QWidget* parent)
    : QDockWidget("Request Blocker", parent) {

  setFeatures(QDockWidget::AllDockWidgetFeatures);

  auto layout = new QVBoxLayout;

  auto top_layout = new QHBoxLayout;
  top_layout->addWidget(new QLabel("Blocked Requests:"), 1);
  auto settings_button = new QToolButton;
  settings_button->setToolTip("Profile Blocker Settings");
  settings_button->setIcon(QIcon(":/res/images/fontawesome/cogs.png"));
  settings_button->setAutoRaise(true);
  top_layout->addWidget(settings_button);
  layout->addLayout(top_layout);

  table_ = new QTableWidget;
  layout->addWidget(table_, 1);

  auto bottom_layout = new QHBoxLayout;
  auto persist_checkbox = new QCheckBox("Persist");
  persist_checkbox->setToolTip("Persist between page loads");
  bottom_layout->addWidget(persist_checkbox);
  auto current_checkbox = new QCheckBox("Current Page");
  current_checkbox->setToolTip("Only show requests for current page");
  bottom_layout->addWidget(current_checkbox);
  bottom_layout->addStretch(1);
  layout->addLayout(bottom_layout);

  auto widg = new QWidget;
  widg->setLayout(layout);
  setWidget(widg);

  ProfileUpdated(cef);
  browser_stack->SetResourceLoadCallback(
        [=](BrowserWidget* browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefRequest> request) -> bool {
    return IsAllowedToLoad(browser, frame, request);
  });
}

void BlockerDock::ProfileUpdated(const Cef& cef) {
  // As a shortcut, if there are no enabled blockers, just null out the
  //  rule set.
  if (Profile::Current()->EnabledBlockerListIds().isEmpty()) {
    next_rule_set_unique_num_++;
    lists_.clear();
    rules_ = nullptr;
    return;
  }

  // We have to be thread safe here. So what we will do is load the list
  //  info here and store in a bunch of local vars and work on those until
  //  the end. We have to make sure we are on the same thread doing work
  //  w/ local members, so we QTimer::singleShot things. We also need a
  //  timeout.
  // This unique number is how we know at the end we're still around.
  next_rule_set_unique_num_++;
  auto my_rule_set_num = next_rule_set_unique_num_;

  // Load up the list into the hash w/ file index. Deleted at the end
  auto new_lists = new QHash<int, BlockerList>();
  auto db_lists =
      BlockerList::Lists(Profile::Current()->EnabledBlockerListIds());
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
          cef, list_index, [=](QList<BlockerRules::Rule*> rules, bool ok) {
      // Single shot to get back on proper thread in event loop
      QTimer::singleShot(0, [=]() {
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

bool BlockerDock::IsAllowedToLoad(BrowserWidget* browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefRequest> request) {
  auto rules = rules_.load();
  if (!rules) return true;
  QElapsedTimer timer;
  timer.start();
  auto target_url_str = QString::fromStdString(request->GetURL().ToString());
  QUrl target_url(target_url_str, QUrl::StrictMode);
  // Ref is same as target if not present
  auto ref_url_str = QString::fromStdString(
        request->GetReferrerURL().ToString());
  QUrl ref_url = ref_url_str.isEmpty() ? target_url :
                                         QUrl(ref_url_str, QUrl::StrictMode);
  auto type = TypeFromRequest(target_url, request);
  auto rule = rules->FindStaticRule(target_url, ref_url, type);
  if (rule) {
    qDebug() << "Found rule for" << target_url_str
             << "on line" << rule->line_num
             << "in" << timer.elapsed() << "ms";
  }
  return rule == nullptr;
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
  }
  if (target_url.scheme() == "ws" || target_url.scheme() == "wss") {
    return BlockerRules::StaticRule::WebSocket;
  }
  // TODO(cretz): Need to support webrtc blocking
  // TODO(cretz): Need to support popup
  return BlockerRules::StaticRule::Other;
}

}  // namespace doogie
