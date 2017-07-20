#include "browser_widget.h"
#include "util.h"

namespace doogie {

BrowserWidget::BrowserWidget(Cef* cef,
                             const QString& url,
                             QWidget* parent)
    : QWidget(parent), cef_(cef) {

  nav_menu_ = new QMenu(this);
  // When this menu is about to be opened we have to populate the items
  connect(nav_menu_, &QMenu::aboutToShow,
          this, &BrowserWidget::RebuildNavMenu);

  back_button_ = new QToolButton();
  back_button_->setText("Back");
  back_button_->setMenu(nav_menu_);
  back_button_->setIcon(Util::CachedIconLighterDisabled(
      ":/res/images/fontawesome/arrow-left.png"));
  back_button_->setAutoRaise(true);
  back_button_->setDisabled(true);
  connect(back_button_, &QToolButton::clicked, [this](bool) { Back(); });

  forward_button_ = new QToolButton();
  forward_button_->setText("Forward");
  forward_button_->setMenu(nav_menu_);
  forward_button_->setIcon(Util::CachedIconLighterDisabled(
      ":/res/images/fontawesome/arrow-right.png"));
  forward_button_->setAutoRaise(true);
  forward_button_->setDisabled(true);
  connect(forward_button_, &QToolButton::clicked, [this](bool) { Forward(); });

  url_edit_ = new QLineEdit(this);
  connect(url_edit_, &QLineEdit::returnPressed, [this]() {
    cef_widg_->LoadUrl(url_edit_->text());
    cef_widg_->setFocus();
  });

  refresh_button_ = new QToolButton();
  refresh_button_->setText("Refresh");
  refresh_button_->setIcon(Util::CachedIconLighterDisabled(
      ":/res/images/fontawesome/repeat.png"));
  refresh_button_->setAutoRaise(true);
  refresh_button_->setDisabled(true);
  connect(refresh_button_, &QToolButton::clicked, [this](bool) { Refresh(); });

  stop_button_ = new QToolButton();
  stop_button_->setText("Refresh");
  stop_button_->setIcon(
        Util::CachedIcon(":/res/images/fontawesome/times-circle.png"));
  stop_button_->setAutoRaise(true);
  stop_button_->setVisible(false);
  connect(stop_button_, &QToolButton::clicked, [this](bool) { Stop(); });

  auto top_layout = new QHBoxLayout();
  top_layout->setMargin(0);
  top_layout->addWidget(back_button_);
  top_layout->addWidget(forward_button_);
  top_layout->addWidget(url_edit_, 1);
  top_layout->addWidget(stop_button_);
  top_layout->addWidget(refresh_button_);
  auto top_widg = new QWidget;
  top_widg->setLayout(top_layout);

  cef_widg_ = new CefWidget(cef, url, this);
  connect(cef_widg_, &CefWidget::PreContextMenu,
          this, &BrowserWidget::BuildContextMenu);
  connect(cef_widg_, &CefWidget::ContextMenuCommand,
          this, &BrowserWidget::HandleContextMenuCommand);
  connect(cef_widg_, &CefWidget::TitleChanged, [this](const QString& title) {
    current_title_ = title;
    emit TitleChanged();
  });
  connect(cef_widg_, &CefWidget::FaviconChanged, [this](const QIcon& icon) {
    current_favicon_ = icon;
    emit FaviconChanged();
  });
  connect(cef_widg_, &CefWidget::LoadStateChanged,
          [this](bool is_loading, bool can_go_back, bool can_go_forward) {
    loading_ = is_loading;
    can_go_back_ = can_go_back;
    can_go_forward_ = can_go_forward;
    refresh_button_->setEnabled(true);
    refresh_button_->setVisible(!is_loading);
    stop_button_->setVisible(is_loading);
    back_button_->setDisabled(!can_go_back);
    forward_button_->setDisabled(!can_go_forward);
    emit LoadingStateChanged();
  });
  connect(cef_widg_, &CefWidget::PageOpen,
          [this](CefHandler::WindowOpenType type,
                 const QString& url,
                 bool user_gesture) {
    emit PageOpen(type, url, user_gesture);
  });
  connect(cef_widg_, &CefWidget::DevToolsLoadComplete,
          this, &BrowserWidget::DevToolsLoadComplete);
  connect(cef_widg_, &CefWidget::DevToolsClosed,
          this, &BrowserWidget::DevToolsClosed);
  connect(cef_widg_, &CefWidget::FindResult,
          this, &BrowserWidget::FindResult);
  connect(cef_widg_, &CefWidget::Closed,
          this, &BrowserWidget::deleteLater);
  connect(cef_widg_, &CefWidget::ShowBeforeUnloadDialog,
          [this](const QString& message_text,
                 bool is_reload,
                 CefRefPtr<CefJSDialogCallback> callback) {
    emit AboutToShowBeforeUnloadDialog();
    auto result = QMessageBox::question(this,
                                        "Leave/Reload Page?",
                                        message_text);
    if (result == QMessageBox::No) {
      emit CloseCancelled();
    }
    callback->Continue(result == QMessageBox::Yes, "");
  });

  auto layout = new QGridLayout;
  layout->addWidget(top_widg, 0, 0);
  layout->addWidget(cef_widg_, 1, 0);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->setRowStretch(0, 0);
  layout->setRowStretch(1, 1);
  setLayout(layout);

  auto override_widg = cef_widg_->OverrideWidget();
  if (override_widg) {
    layout->addWidget(override_widg, 1, 0);
  }

  connect(cef_widg_, &CefWidget::UrlChanged, [this](const QString& url) {
    url_edit_->setText(url);
  });

  find_widg_ = new FindWidget(this);
  layout->addWidget(find_widg_);
  find_widg_->setVisible(false);
  connect(find_widg_, &FindWidget::AttemptFind,
          [this](const QString& text, bool forward,
                 bool match_case, bool continued) {
    if (!text.isEmpty()) {
      cef_widg_->Find(text, forward, match_case, continued);
    } else {
      cef_widg_->CancelFind(true);
    }
  });
  connect(find_widg_, &FindWidget::Hidden, [this]() {
    UpdateStatusBarLocation();
    cef_widg_->CancelFind(true);
  });
  connect(this, &BrowserWidget::FindResult,
          find_widg_, &FindWidget::FindResult);

  status_bar_ = new QLabel(this);
  status_bar_->hide();
  status_bar_->resize(300, status_bar_->height());
  this->UpdateStatusBarLocation();
  connect(cef_widg_, &CefWidget::StatusChanged, [this](const QString& status) {
    if (status.isEmpty()) {
      status_bar_->hide();
    } else {
      int change_count = status_bar_->property("change_count").toInt();
      status_bar_->setProperty("change_count", ++change_count);
      status_bar_->setProperty("full_text", status);
      status_bar_->resize(300, status_bar_->height());
      // Add ellipsis
      status_bar_->setText(status_bar_->fontMetrics().elidedText(
          status, Qt::ElideRight, status_bar_->width()));
      UpdateStatusBarLocation();
      status_bar_->show();
      // Show the full message after they stay there a couple of seconds
      QTimer::singleShot(2000, Qt::CoarseTimer, status_bar_,
                         [this, change_count]() {
        if (status_bar_->isVisible() &&
            status_bar_->property("change_count").toInt() == change_count) {
          status_bar_->resize(width(), status_bar_->height());
          // Add ellipsis
          status_bar_->setText(status_bar_->fontMetrics().elidedText(
              status_bar_->property("full_text").toString(),
              Qt::ElideRight, status_bar_->width()));
          UpdateStatusBarLocation();
        }
      });
    }
  });
}

void BrowserWidget::LoadUrl(const QString &url) {
  cef_widg_->LoadUrl(url);
}

void BrowserWidget::TryClose() {
  cef_widg_->TryClose();
}

void BrowserWidget::FocusUrlEdit() {
  url_edit_->setFocus();
  url_edit_->activateWindow();
  // We choose to select the entire text here to make nav easier
  url_edit_->selectAll();
}

void BrowserWidget::FocusBrowser() {
  cef_widg_->setFocus();
}

QIcon BrowserWidget::CurrentFavicon() {
  return current_favicon_;
}

QString BrowserWidget::CurrentTitle() {
  return current_title_;
}

QString BrowserWidget::CurrentUrl() {
  return cef_widg_->CurrentUrl();
}

bool BrowserWidget::Loading() {
  return loading_;
}

bool BrowserWidget::CanGoBack() {
  return can_go_back_;
}

bool BrowserWidget::CanGoForward() {
  return can_go_forward_;
}

void BrowserWidget::Refresh() {
  auto ignore_cache = QApplication::keyboardModifiers().
      testFlag(Qt::ControlModifier);
  cef_widg_->Refresh(ignore_cache);
}

void BrowserWidget::Stop() {
  cef_widg_->Stop();
}

void BrowserWidget::Back() {
  cef_widg_->Go(-1);
}

void BrowserWidget::Forward() {
  cef_widg_->Go(1);
}

void BrowserWidget::Print() {
  cef_widg_->Print();
}

void BrowserWidget::ShowFind() {
  find_widg_->show();
  find_widg_->setFocus();
  UpdateStatusBarLocation();
}

void BrowserWidget::ExecJs(const QString &js) {
  cef_widg_->ExecJs(js);
}

void BrowserWidget::ShowDevTools(CefBaseWidget* widg,
                                 const QPoint& inspect_at) {
  cef_widg_->ShowDevTools(widg, inspect_at);
}

void BrowserWidget::ExecDevToolsJs(const QString& js) {
  cef_widg_->ExecDevToolsJs(js);
}

void BrowserWidget::CloseDevTools() {
  cef_widg_->CloseDevTools();
}

double BrowserWidget::GetZoomLevel() {
  return cef_widg_->GetZoomLevel();
}

void BrowserWidget::SetZoomLevel(double level) {
  cef_widg_->SetZoomLevel(level);
}

QJsonObject BrowserWidget::DebugDump() {
  return {
    { "loading", loading_ },
    { "rect", Util::DebugWidgetGeom(this) },
    { "urlEdit", QJsonObject({
      { "text", url_edit_->text() },
      { "focus", url_edit_->hasFocus() },
      { "rect", Util::DebugWidgetGeom(url_edit_) }
    })},
    { "main", QJsonObject({
      { "focus", cef_widg_->hasFocus() },
      { "rect", Util::DebugWidgetGeom(cef_widg_) }
    })},
    { "statusBar", QJsonObject({
      { "visible", status_bar_->isVisible() },
      { "visibleText", status_bar_->text() },
      { "rect", Util::DebugWidgetGeom(status_bar_) }
    })}
  };
}

void BrowserWidget::moveEvent(QMoveEvent*) {
  this->UpdateStatusBarLocation();
}

void BrowserWidget::resizeEvent(QResizeEvent*) {
  this->UpdateStatusBarLocation();
}

void BrowserWidget::UpdateStatusBarLocation() {
  auto new_y = this->height() - status_bar_->height();
  if (find_widg_->isVisible()) new_y -= find_widg_->height();
  status_bar_->move(0, new_y);
}

void BrowserWidget::RebuildNavMenu() {
  nav_menu_->clear();
  auto entries = cef_widg_->NavEntries();
  // We need to find the "current" index so we can do neg/pos index from that
  int current_item_index = 0;
  for (int i = 0; i < entries.size(); i++) {
    if (entries[i].current) {
      current_item_index = i;
      break;
    }
  }
  // Now that we have the current, we know the index to go back or forward
  // We have to go backwards...
  for (int i = static_cast<int>(entries.size()) - 1; i >= 0; i--) {
    auto entry = entries[i];
    auto action = nav_menu_->addAction(entry.title);
    auto nav_index = i - current_item_index;
    if (nav_index != 0) {
      connect(action, &QAction::triggered, [this, nav_index]() {
        cef_widg_->Go(nav_index);
      });
    } else {
      // Bold the current
      auto new_font = action->font();
      new_font.setBold(true);
      action->setFont(new_font);
    }
  }
}

void BrowserWidget::BuildContextMenu(CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) {
  if (params->GetTypeFlags() & CM_TYPEFLAG_LINK) {
    model->InsertItemAt(0, ContextMenuOpenLinkChildPage, "Open in Child Page");
    model->InsertItemAt(1, ContextMenuOpenLinkChildPageBackground,
                   "Open in Background Child Page");
    model->InsertItemAt(2, ContextMenuOpenLinkTopLevelPage,
                   "Open in Top-Level Page");
    model->InsertItemAt(3, ContextMenuCopyLinkAddress,
                   "Copy Link Address");
    model->InsertSeparatorAt(4);
  }
  model->AddSeparator();
  model->Remove(MENU_ID_VIEW_SOURCE);
  model->AddItem(ContextMenuViewPageSource, "View Page Source");
  if (params->GetTypeFlags() & CM_TYPEFLAG_FRAME) {
    model->AddItem(ContextMenuViewFrameSource, "View Frame Source");
  }
  model->AddItem(ContextMenuInspectElement, "Inspect");
}

void BrowserWidget::HandleContextMenuCommand(
    CefRefPtr<CefContextMenuParams> params,
    int command_id,
    CefContextMenuHandler::EventFlags event_flags) {
  switch (command_id) {
    case ContextMenuOpenLinkChildPage: {
      auto url = QString::fromStdString(params->GetLinkUrl().ToString());
      if (event_flags & EVENTFLAG_CONTROL_DOWN) {
        emit PageOpen(CefHandler::OpenTypeNewBackgroundTab, url, true);
      } else {
        emit PageOpen(CefHandler::OpenTypeNewForegroundTab, url, true);
      }
      break;
    }
    case ContextMenuOpenLinkChildPageBackground: {
      emit PageOpen(CefHandler::OpenTypeNewBackgroundTab,
                    QString::fromStdString(params->GetLinkUrl().ToString()),
                    true);
      break;
    }
    case ContextMenuOpenLinkTopLevelPage: {
      emit PageOpen(CefHandler::OpenTypeNewWindow,
                    QString::fromStdString(params->GetLinkUrl().ToString()),
                    true);
      break;
    }
    case ContextMenuCopyLinkAddress: {
      QGuiApplication::clipboard()->setText(
            QString::fromStdString(params->GetUnfilteredLinkUrl().ToString()));
      break;
    }
    case ContextMenuInspectElement: {
      emit ShowDevToolsRequest(QPoint(params->GetXCoord(),
                                      params->GetYCoord()));
      break;
    }
    case ContextMenuViewPageSource: {
      auto url = QString("view-source:") +
          QString::fromStdString(params->GetPageUrl().ToString());
      if (event_flags & EVENTFLAG_CONTROL_DOWN) {
        emit PageOpen(CefHandler::OpenTypeNewBackgroundTab, url, true);
      } else {
        emit PageOpen(CefHandler::OpenTypeNewForegroundTab, url, true);
      }
      break;
    }
  case ContextMenuViewFrameSource: {
    auto url = QString("view-source:") +
        QString::fromStdString(params->GetFrameUrl().ToString());
    if (event_flags & EVENTFLAG_CONTROL_DOWN) {
      emit PageOpen(CefHandler::OpenTypeNewBackgroundTab, url, true);
    } else {
      emit PageOpen(CefHandler::OpenTypeNewForegroundTab, url, true);
    }
    break;
  }
    default: {
      throw std::invalid_argument("Unknown command");
    }
  }
}

}  // namespace doogie
