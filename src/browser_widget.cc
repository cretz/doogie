#include "browser_widget.h"

#include "page_index.h"
#include "util.h"
#include "ssl_info_action.h"

namespace doogie {

BrowserWidget::BrowserWidget(const Cef& cef,
                             const Bubble& bubble,
                             const QString& url,
                             QWidget* parent)
    : QWidget(parent), cef_(cef), bubble_(bubble) {

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
  connect(back_button_, &QToolButton::clicked, [=](bool) { Back(); });

  forward_button_ = new QToolButton();
  forward_button_->setText("Forward");
  forward_button_->setMenu(nav_menu_);
  forward_button_->setIcon(Util::CachedIconLighterDisabled(
      ":/res/images/fontawesome/arrow-right.png"));
  forward_button_->setAutoRaise(true);
  forward_button_->setDisabled(true);
  connect(forward_button_, &QToolButton::clicked, [=](bool) { Forward(); });

  ssl_button_ = new QToolButton;
  ssl_button_->setAutoRaise(true);
  ssl_button_->setDisabled(true);
  ssl_button_->setPopupMode(QToolButton::InstantPopup);
  ssl_button_->addAction(new SslInfoAction(cef_, this));

  url_edit_ = new UrlEdit(cef, this);
  connect(url_edit_, &UrlEdit::UrlEntered, [=]() {
    cef_widg_->LoadUrl(url_edit_->text());
    cef_widg_->setFocus();
  });

  refresh_button_ = new QToolButton();
  refresh_button_->setText("Refresh");
  refresh_button_->setIcon(Util::CachedIconLighterDisabled(
      ":/res/images/fontawesome/repeat.png"));
  refresh_button_->setAutoRaise(true);
  refresh_button_->setDisabled(true);
  connect(refresh_button_, &QToolButton::clicked, [=](bool) { Refresh(); });

  stop_button_ = new QToolButton();
  stop_button_->setText("Refresh");
  stop_button_->setIcon(
        Util::CachedIcon(":/res/images/fontawesome/times-circle.png"));
  stop_button_->setAutoRaise(true);
  stop_button_->setVisible(false);
  connect(stop_button_, &QToolButton::clicked, [=](bool) { Stop(); });

  auto top_layout = new QHBoxLayout();
  top_layout->setMargin(0);
  top_layout->addWidget(back_button_);
  top_layout->addWidget(forward_button_);
  top_layout->addWidget(ssl_button_);
  top_layout->addWidget(url_edit_, 1);
  top_layout->addWidget(stop_button_);
  top_layout->addWidget(refresh_button_);
  auto top_widg = new QWidget;
  top_widg->setLayout(top_layout);

  // Initial suggested size is the parent minus the top widget's hinted height
  QSize initial_size;
  if (parent) {
    initial_size = QSize(parent->size().width(),
                         parent->size().height() -
                         top_widg->sizeHint().height());
  }
  RecreateCefWidget(url, initial_size);

  auto layout = new QGridLayout;
  layout->addWidget(top_widg, 0, 0);
  layout->addWidget(cef_widg_, 1, 0);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->setRowStretch(0, 0);
  layout->setRowStretch(1, 1);
  setLayout(layout);

  cef_widg_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  cef_widg_->adjustSize();

  // We set the view widget afterwards because it is not ready before now
  layout->addWidget(cef_widg_->ViewWidget(), 1, 0);

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
  UpdateStatusBarLocation();

  connect(this, &BrowserWidget::SuspensionChanged,
          this, &BrowserWidget::ShowAsSuspendedScreenshot);
}

void BrowserWidget::LoadUrl(const QString &url) {
  // If this starts with a data URI for text/html, we are going to be lazy
  // and assume it's for an error page for now and not change the URL.
  // TODO: Stop being lazy (and opening security holes) and show errors better
  if (!url.startsWith("data:text/html;base64,")) url_edit_->setText(url);
  cef_widg_->LoadUrl(url);
}

void BrowserWidget::SetUrlText(const QString& url) {
  url_edit_->setText(url);
}

void BrowserWidget::TryClose() {
  // Set not suspended so this can die
  suspended_ = false;
  cef_widg_->TryClose();
}

void BrowserWidget::FocusUrlEdit() {
  // We have to defer this because CEF window steals focus
  // immediately here.
  // TODO(cretz): Fix when the following is fixed:
  //  https://bitbucket.org/chromiumembedded/cef/issues/1856/
  QTimer::singleShot(0, [=]() {
    url_edit_->activateWindow();
    url_edit_->setFocus();
    // We choose to select the entire text here to make nav easier
    url_edit_->selectAll();
  });
}

void BrowserWidget::FocusBrowser() {
  // We clear the focus here first because sometimes this is called
  //  when it already has focus, but we need to trigger a "change"
  //  so the underlying piece gets focus.
  cef_widg_->clearFocus();
  cef_widg_->setFocus();
}

const Bubble& BrowserWidget::CurrentBubble() const {
  return bubble_;
}

void BrowserWidget::ChangeCurrentBubble(const Bubble& bubble) {
  bubble_ = bubble;
  emit BubbleMaybeChanged();
  RecreateCefWidget(CurrentUrl());
}

QIcon BrowserWidget::CurrentFavicon() const {
  return current_favicon_;
}

QString BrowserWidget::CurrentTitle() const {
  return current_title_;
}

QString BrowserWidget::CurrentUrl() const {
  // As a special case, if this URL is invalid, we use
  //  what's in the URL edit box
  auto url = cef_widg_->CurrentUrl();
  if (url.startsWith("data:text/html")) {
    url = url_edit_->text();
  }
  return url;
}

bool BrowserWidget::Loading() const {
  return loading_;
}

bool BrowserWidget::CanGoBack() const {
  return can_go_back_;
}

bool BrowserWidget::CanGoForward() const {
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

void BrowserWidget::Fullscreen() {
  if (!cef_widg_->isFullScreen()) {
    cef_widg_->ApplyFullscreen(true);
  }
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

double BrowserWidget::ZoomLevel() const {
  return cef_widg_->ZoomLevel();
}

void BrowserWidget::SetZoomLevel(double level) {
  cef_widg_->SetZoomLevel(level);
}

bool BrowserWidget::Suspended() const {
  return suspended_;
}

void BrowserWidget::SetSuspended(bool suspend, const QString& url_override) {
  if (suspended_ == suspend) return;
  // We have to defer to let loading state change
  suspended_ = suspend;
  if (suspended_) {
    suspended_url_ = url_override.isNull() ? CurrentUrl() : url_override;
    // We will make fake pixels for the rest of the screen
    // and we will make it lighter to look disabled
    auto screen = QGuiApplication::primaryScreen();
    suspended_screenshot_ = QPixmap(screen->size());
    // Only do this if this is visible
    if (cef_widg_->isVisible()) {
      Util::LighterDisabled(screen->grabWindow(cef_widg_->winId()),
                            &suspended_screenshot_);
    }
    QPainter painter;
    painter.begin(&suspended_screenshot_);
    auto font = painter.font();
    font.setPointSize(40);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(rect(), Qt::AlignCenter, "Suspended");
    // Close directly, don't call this->TryClose
    cef_widg_->TryClose();
  } else {
    emit SuspensionChanged();
    cef_widg_->LoadUrl(suspended_url_);
    cef_widg_->setFocus();
  }
}

void BrowserWidget::SetResourceLoadCallback(ResourceLoadCallback callback) {
  if (!callback) {
    resource_load_callback_ = nullptr;
  } else {
    resource_load_callback_ = [=](CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefRequest> request) -> bool {
      return callback(this, frame, request);
    };
  }
  cef_widg_->SetResourceLoadCallback(resource_load_callback_);
}

QJsonObject BrowserWidget::DebugDump() const {
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

void BrowserWidget::moveEvent(QMoveEvent* event) {
  UpdateStatusBarLocation();
  QWidget::moveEvent(event);
}

void BrowserWidget::resizeEvent(QResizeEvent* event) {
  UpdateStatusBarLocation();
  QWidget::resizeEvent(event);
}

void BrowserWidget::RecreateCefWidget(const QString& url,
                                      const QSize& initial_size) {
  // Delete the other one if it's there
  QWidget* widg_to_replace = nullptr;
  QSize widg_size = initial_size;
  if (cef_widg_) {
    // We override the size with the old size here
    widg_size = cef_widg_->size();
    widg_to_replace = cef_widg_->ViewWidget();
    cef_widg_->disconnect();
    cef_widg_->deleteLater();
  }
  cef_widg_ = new CefWidget(cef_, bubble_, url, this, widg_size);
  cef_widg_->SetResourceLoadCallback(resource_load_callback_);
  connect(cef_widg_, &CefWidget::PreContextMenu,
          this, &BrowserWidget::BuildContextMenu);
  connect(cef_widg_, &CefWidget::ContextMenuCommand,
          this, &BrowserWidget::HandleContextMenuCommand);
  connect(cef_widg_, &CefWidget::TitleChanged, [=](const QString& title) {
    current_title_ = title;
    emit TitleChanged();
    PageIndex::UpdateTitle(CurrentUrl(), title);
  });
  connect(cef_widg_, &CefWidget::FaviconChanged,
          [=](const QString& url, const QIcon& icon) {
    current_favicon_url_ = url;
    current_favicon_ = icon;
    emit FaviconChanged();
    PageIndex::UpdateFavicon(CurrentUrl(), url, icon);
  });
  connect(cef_widg_, &CefWidget::DownloadRequested,
          this, &BrowserWidget::DownloadRequested);
  connect(cef_widg_, &CefWidget::DownloadUpdated,
          this, &BrowserWidget::DownloadUpdated);
  connect(cef_widg_, &CefWidget::LoadStateChanged,
          [=](bool is_loading, bool can_go_back, bool can_go_forward) {
    loading_ = is_loading;
    can_go_back_ = can_go_back;
    can_go_forward_ = can_go_forward;
    refresh_button_->setEnabled(true);
    refresh_button_->setVisible(!is_loading);
    stop_button_->setVisible(is_loading);
    back_button_->setDisabled(!can_go_back);
    forward_button_->setDisabled(!can_go_forward);
    // It's no longer suspended
    if (suspended_ && loading_) {
      suspended_ = false;
      emit SuspensionChanged();
    }
    emit LoadingStateChanged();
    // If loading is complete, mark the visit. We used to do this on
    //  load start and check the transition type, but we realized that
    //  it's best to wait until it's finished anyways and we don't care
    //  as much about the transition type.
    // TODO(cretz): Test when title/favicon is not changed or not present
    if (!is_loading) {
      auto title = current_title_.isNull() ? "" : current_title_;
      PageIndex::MarkVisit(CurrentUrl(), title,
                           current_favicon_url_, current_favicon_);
    }

    // Reset the stylesheet and update SSL status when not errored
    if (number_of_load_completes_are_error_ <= 0) {
      UpdateSslStatus(false);
      url_edit_->setStyleSheet("");
    } else if (!loading_) {
      number_of_load_completes_are_error_--;
    }
  });
  connect(cef_widg_, &CefWidget::LoadError,
          [=](CefRefPtr<CefFrame> frame,
              CefLoadHandler::ErrorCode error_code,
              const QString& error_text,
              const QString& failed_url) {
    // Some error codes we are ok with
    if (error_code != ERR_NONE && error_code != ERR_ABORTED) {
      ShowError(failed_url, error_text, frame);
    }
  });
  connect(cef_widg_, &CefWidget::CertificateError,
          [=](cef_errorcode_t,
              const QString& request_url,
              CefRefPtr<CefSSLInfo> ssl_info,
              CefRefPtr<CefRequestCallback> callback) {
    // TODO(cretz): Check if this is the main frame please
    errored_ssl_info_ = ssl_info;
    errored_ssl_callback_ = callback;
    ShowError(request_url, "Invalid Certificate");
    UpdateSslStatus(true);
  });
  connect(cef_widg_, &CefWidget::PageOpen,
          [=](CefHandler::WindowOpenType type,
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
  connect(cef_widg_, &CefWidget::Closed, [this]() {
    // Only delete this if we're not suspended, otherwise replace
    if (suspended_) {
      RecreateCefWidget("");
      emit SuspensionChanged();
    } else {
      delete this;
    }
  });
  cef_widg_->SetJsDialogCallback(
        [=](const QString&,
            CefJSDialogHandler::JSDialogType dialog_type,
            const QString& message_text,
            const QString& default_prompt_text,
            CefRefPtr<CefJSDialogCallback> callback,
            bool&) {
    emit AboutToShowJSDialog();
    switch (dialog_type) {
      case JSDIALOGTYPE_ALERT:
        QMessageBox::information(this, "[JavaScript Alert]", message_text);
        callback->Continue(true, "");
        break;
      case JSDIALOGTYPE_CONFIRM: {
        auto result = QMessageBox::question(
              this, "[JavaScript Confirm]", message_text,
              QMessageBox::Ok | QMessageBox::Cancel);
        callback->Continue(result == QMessageBox::Ok, "");
        break;
      }
      case JSDIALOGTYPE_PROMPT: {
        auto result = QInputDialog::getText(
              this, "[JavaScript Prompt]", message_text,
              QLineEdit::Normal, default_prompt_text);
        if (result.isNull()) {
          callback->Continue(false, "");
        } else {
          callback->Continue(true, CefString(result.toStdString()));
        }
        break;
      }
      default:
        callback->Continue(false, "");
    }
  });
  connect(cef_widg_, &CefWidget::ShowBeforeUnloadDialog,
          [=](const QString& message_text,
              bool,
              CefRefPtr<CefJSDialogCallback> callback) {
    emit AboutToShowJSDialog();
    auto result = QMessageBox::question(this,
                                        "Leave/Reload Page?",
                                        message_text);
    if (result == QMessageBox::No) {
      // Can't be suspended if close cancelled
      suspended_ = false;
      emit CloseCancelled();
    }
    callback->Continue(result == QMessageBox::Yes, "");
  });
  connect(cef_widg_, &CefWidget::StatusChanged, [=](const QString& status) {
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
  connect(cef_widg_, &CefWidget::UrlChanged, [=](const QString& url) {
    // If this starts with a data URI for text/html, we are going to be lazy
    // and assume it's for an error page for now and not change the URL.
    // TODO: Stop being lazy (and opening security holes) and show errors better
    if (!url.startsWith("data:text/html;base64,")) url_edit_->setText(url);
  });

  if (widg_to_replace) {
    layout()->replaceWidget(widg_to_replace, cef_widg_->ViewWidget());
  }
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
  for (size_t i = 0; i < entries.size(); i++) {
    if (entries[i].current) {
      current_item_index = static_cast<int>(i);
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
      connect(action, &QAction::triggered, [=]() {
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

void BrowserWidget::ShowAsSuspendedScreenshot() {
  auto grid_layout = qobject_cast<QGridLayout*>(layout());
  if (Suspended()) {
    // Have to disable the SSL button
    ssl_button_->setDisabled(true);
    // Remove the cef widg and add screenshot
    grid_layout->removeWidget(cef_widg_);
    cef_widg_->hide();
    auto widg = new QWidget();
    widg->setAutoFillBackground(true);
    auto pal = widg->palette();
    pal.setBrush(QPalette::Window, QBrush(suspended_screenshot_));
    widg->setPalette(pal);
    grid_layout->addWidget(widg, 1, 0);
  } else {
    suspended_screenshot_ = QPixmap();
    // Remove the screenshot and add back the cef widg
    auto item = grid_layout->itemAtPosition(1, 0);
    grid_layout->removeItem(item);
    if (item->widget()) {
      delete item->widget();
    }
    delete item;
    cef_widg_->show();
    grid_layout->addWidget(cef_widg_, 1, 0);
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

void BrowserWidget::ShowError(const QString& /*failed_url*/,
                              const QString& error_text,
                              CefRefPtr<CefFrame> frame) {
  if (!frame || frame->IsMain()) {
    url_edit_->setStyleSheet("QLineEdit { background-color: pink; }");
    number_of_load_completes_are_error_ = 2;
  }
  auto new_html =
      QString("<html><head><title>(error)</title></head>"
              "<body style=\"background-color: pink;\">"
              "Error loading page: <strong>%1</strong>"
              "</body></html>").arg(error_text);
  cef_widg_->ShowStringPage(new_html, "text/html", frame);
}

void BrowserWidget::UpdateSslStatus(bool check_errored) {
  if (check_errored && errored_ssl_info_) {
    ssl_button_->setDisabled(false);
    ssl_button_->setIcon(QIcon(*Util::CachedPixmapColorOverlay(
        ":/res/images/fontawesome/unlock.png", "red")));
    ssl_button_->setToolTip("Invalid Certificate");
  } else if (loading_) {
    ssl_button_->setDisabled(true);
    ssl_button_->setIcon(QIcon());
    ssl_button_->setToolTip("");
  } else {
    errored_ssl_info_ = nullptr;
    errored_ssl_callback_ = nullptr;
    ssl_status_ = cef_widg_->CurrentSSLStatus();
    // If the SSL status is not there or there is no SSL, we mark
    //  it as unlocked and disable the button
    if (!ssl_status_ || !ssl_status_->IsSecureConnection()) {
      ssl_button_->setIcon(Util::CachedIconLighterDisabled(
          ":/res/images/fontawesome/unlock-alt.png"));
      ssl_button_->setDisabled(true);
      ssl_button_->setToolTip("Not secure");
    } else {
      ssl_button_->setDisabled(false);
      // Red when there is a cert error, orange for content error,
      //  green otherwise
      if (ssl_status_->GetCertStatus() > CERT_STATUS_NONE
          && ssl_status_->GetCertStatus() < CERT_STATUS_IS_EV) {
        ssl_button_->setIcon(QIcon(*Util::CachedPixmapColorOverlay(
            ":/res/images/fontawesome/unlock.png", "red")));
        ssl_button_->setToolTip("Invalid Certificate");
      } else if (ssl_status_->GetContentStatus() !=
                 SSL_CONTENT_NORMAL_CONTENT) {
        ssl_button_->setIcon(QIcon(*Util::CachedPixmapColorOverlay(
            ":/res/images/fontawesome/unlock.png", "orange")));
        ssl_button_->setToolTip("Insecure Page Contents");
      } else {
        ssl_button_->setIcon(QIcon(*Util::CachedPixmapColorOverlay(
            ":/res/images/fontawesome/lock.png", "green")));
        ssl_button_->setToolTip("Secure");
      }
    }
  }
}

}  // namespace doogie
