#include "cef/cef_widget.h"

#include <string>

namespace doogie {

CefWidget::CefWidget(const Cef& cef,
                     const Bubble& bubble,
                     const QString& url,
                     QWidget* parent,
                     const QSize& initial_size)
    : CefBaseWidget(cef, parent, initial_size) {
  handler_ = CefRefPtr<CefHandler>(new CefHandler);
  ForwardKeyboardEventsFrom(handler_);
  setFocusPolicy(Qt::StrongFocus);
  handler_->SetFocusAllowedCallback([=]() { return hasFocus(); });
  connect(handler_, &CefHandler::PreContextMenu,
          this, &CefWidget::PreContextMenu);
  connect(handler_, &CefHandler::ContextMenuCommand,
          this, &CefWidget::ContextMenuCommand);
  connect(handler_, &CefHandler::UrlChanged,
          this, &CefWidget::UrlChanged);
  connect(handler_, &CefHandler::TitleChanged,
          this, &CefWidget::TitleChanged);
  connect(handler_, &CefHandler::StatusChanged,
          this, &CefWidget::StatusChanged);
  connect(handler_, &CefHandler::Closed,
          this, &CefWidget::Closed);
  connect(handler_, &CefHandler::FaviconUrlChanged, [=](const QString& url) {
    static const auto favicon_sig =
        QMetaMethod::fromSignal(&CefWidget::FaviconChanged);
    if (this->isSignalConnected(favicon_sig) && browser_) {
      // Download the favicon
      browser_->GetHost()->DownloadImage(
            CefString(url.toStdString()),
            true,
            16,
            false,
            new CefWidget::FaviconDownloadCallback(this));
    }
  });
  connect(handler_, &CefHandler::FullscreenModeChange, [=](bool fullscreen) {
    js_triggered_fullscreen_ = fullscreen;
    ApplyFullscreen(fullscreen);
  });
  connect(handler_, &CefHandler::DownloadRequested,
          this, &CefWidget::DownloadRequested);
  connect(handler_, &CefHandler::DownloadUpdated,
          this, &CefWidget::DownloadUpdated);
  connect(handler_, &CefHandler::FocusObtained, [=]() {
    setFocus();
  });
  connect(handler_, &CefHandler::LoadStateChanged,
          this, &CefWidget::LoadStateChanged);
  connect(handler_, &CefHandler::LoadStart,
          this, &CefWidget::LoadStart);
  connect(handler_, &CefHandler::LoadError,
          this, &CefWidget::LoadError);
  connect(handler_, &CefHandler::CertificateError,
          this, &CefWidget::CertificateError);
  connect(handler_, &CefHandler::PageOpen,
          this, &CefWidget::PageOpen);
  connect(handler_, &CefHandler::FindResult,
          [=](int, int count, const CefRect&,
              int active_match_ordinal, bool) {
    FindResult(count, active_match_ordinal);
  });
  connect(handler_, &CefHandler::ShowBeforeUnloadDialog,
          this, &CefWidget::ShowBeforeUnloadDialog);
  connect(handler_, &CefHandler::AuthRequest,
          [=](const QString&, bool, const QString&,
              int, const QString& realm, const QString&,
              CefRefPtr<CefAuthCallback> callback) {
    Util::RunOnMainThread([=]() { AuthRequest(realm, callback); });
  });

  InitBrowser(bubble, url);
}

CefWidget::~CefWidget() {
  if (browser_) {
    browser_->GetHost()->CloseBrowser(true);
  }
}

std::vector<CefWidget::NavEntry> CefWidget::NavEntries() const {
  CefRefPtr<CefWidget::NavEntryVisitor> visitor =
      new CefWidget::NavEntryVisitor();
  if (browser_) {
    browser_->GetHost()->GetNavigationEntries(visitor, false);
  }
  return visitor->Entries();
}

CefRefPtr<CefSSLStatus> CefWidget::CurrentSSLStatus() const {
  CefRefPtr<CefWidget::NavEntryCurrentSslVisitor> visitor =
      new CefWidget::NavEntryCurrentSslVisitor();
  if (browser_ && !browser_->IsLoading() && browser_->HasDocument()) {
    browser_->GetHost()->GetNavigationEntries(visitor, true);
  }
  return visitor->SslStatus();
}

void CefWidget::LoadUrl(const QString& url) {
  if (browser_) {
    browser_->GetMainFrame()->LoadURL(CefString(url.toStdString()));
  }
}

void CefWidget::ShowStringPage(const QString& data,
                               const QString& mime_type,
                               CefRefPtr<CefFrame> frame) {
  if (browser_) {
    auto f = frame ? frame : browser_->GetMainFrame();
    // Ref https://bitbucket.org/chromiumembedded/cef/issues/2586/loadstring-results-in-terminating-renderer
    // We must use LoadURL with a data URL
    std::string data_str = data.toStdString();
    std::string base64 = CefURIEncode(CefBase64Encode(data_str.data(), data_str.size()), false).ToString();
    std::string data_uri = "data:" + mime_type.toStdString() + ";base64," + base64;
    f->LoadURL(data_uri);
  }
}

QString CefWidget::CurrentUrl() const {
  if (browser_) {
    return QString::fromStdString(
          browser_->GetMainFrame()->GetURL().ToString());
  }
  return "";
}

bool CefWidget::HasDocument() const {
  return browser_ && browser_->HasDocument();
}

void CefWidget::TryClose() {
  if (browser_) {
    browser_->GetHost()->CloseBrowser(true);
  }
}

void CefWidget::Go(int num) {
  if (browser_) {
    browser_->GetMainFrame()->ExecuteJavaScript(
        CefString(std::string("history.go(") + std::to_string(num) + ")"),
        CefString("<doogie>"),
        0);
  }
}

void CefWidget::Refresh(bool ignore_cache) {
  if (browser_) {
    if (ignore_cache) {
      browser_->ReloadIgnoreCache();
    } else {
      browser_->Reload();
    }
  }
}

void CefWidget::Stop() {
  if (browser_) {
    browser_->StopLoad();
  }
}

void CefWidget::Print() {
  if (browser_) {
    browser_->GetHost()->Print();
  }
}

void CefWidget::Find(const QString& text,
                     bool forward,
                     bool match_case,
                     bool continued) {
  if (browser_) {
    browser_->GetHost()->Find(0, CefString(text.toStdString()),
                              forward, match_case, continued);
  }
}

void CefWidget::CancelFind(bool clear_selection) {
  if (browser_) {
    browser_->GetHost()->StopFinding(clear_selection);
  }
}

void CefWidget::ExecJs(const QString &js) {
  if (browser_) {
    browser_->GetMainFrame()->ExecuteJavaScript(
          CefString(js.toStdString()), "<doogie>", 0);
  }
}

void CefWidget::ShowDevTools(CefBaseWidget* widg,
                             const QPoint& inspect_at) {
  if (browser_) {
    CefBrowserSettings settings;
    if (!dev_tools_handler_) {
      dev_tools_handler_ = CefRefPtr<CefHandler>(new CefHandler);
      widg->ForwardKeyboardEventsFrom(dev_tools_handler_);
      connect(dev_tools_handler_, &CefHandler::AfterCreated,
              [=](CefRefPtr<CefBrowser> b) {
        dev_tools_browser_ = b;
      });
      connect(dev_tools_handler_, &CefHandler::LoadEnd,
              [=](CefRefPtr<CefFrame> frame, int) {
        if (frame->IsMain()) emit DevToolsLoadComplete();
      });
      connect(dev_tools_handler_, &CefHandler::Closed, [=]() {
        CloseDevTools();
      });
    }
    browser_->GetHost()->ShowDevTools(
          widg->WindowInfo(),
          dev_tools_handler_,
          settings,
          CefPoint(inspect_at.x(), inspect_at.y()));
  }
}

void CefWidget::ExecDevToolsJs(const QString& js) {
  if (dev_tools_browser_) {
    dev_tools_browser_->GetMainFrame()->ExecuteJavaScript(
          CefString(js.toStdString()), "<doogie>", 0);
  }
}

void CefWidget::CloseDevTools() {
  // We have to nullify the dev tools browser to make this reentrant
  // Sadly, HasDevTools is not false after CloseDevTools
  if (dev_tools_browser_) {
    dev_tools_browser_->GetHost()->CloseDevTools();
    dev_tools_browser_ = nullptr;
    emit DevToolsClosed();
  }
}

double CefWidget::ZoomLevel() const {
  if (browser_) {
    return browser_->GetHost()->GetZoomLevel();
  }
  return 0.0;
}

void CefWidget::SetZoomLevel(double level) {
  if (browser_) {
    browser_->GetHost()->SetZoomLevel(level);
  }
}

void CefWidget::SetJsDialogCallback(CefHandler::JsDialogCallback callback) {
  handler_->SetJsDialogCallback(callback);
}

void CefWidget::SetResourceLoadCallback(
    CefHandler::ResourceLoadCallback callback) {
  handler_->SetResourceLoadCallback(callback);
}

void CefWidget::ApplyFullscreen(bool fullscreen) {
  if (fullscreen) {
    setWindowFlags(windowFlags() | Qt::Window);
    showFullScreen();
  } else {
    js_triggered_fullscreen_ = false;
    setWindowFlags(windowFlags() & (~Qt::Window));
    showNormal();
  }
}

void CefWidget::focusInEvent(QFocusEvent* event) {
  QWidget::focusInEvent(event);
  if (browser_) {
    browser_->GetHost()->SetFocus(true);
  }
}

void CefWidget::focusOutEvent(QFocusEvent* event) {
  QWidget::focusOutEvent(event);
  if (browser_) {
    browser_->GetHost()->SetFocus(false);
  }
}

void CefWidget::keyPressEvent(QKeyEvent* event) {
  // Exit fullscreen if currently in it and pressing f11 or esc
  if ((event->key() == Qt::Key_Escape || event->key() == Qt::Key_F11) &&
      isFullScreen()) {
    // If it was JS triggered, we have to leave that mode instead
    // TODO(cretz): Use non-JS method to escape fullscreen some day.
    if (js_triggered_fullscreen_) {
      ExecJs("document.webkitExitFullscreen();");
    } else {
      ApplyFullscreen(false);
    }
    return;
  }
  QWidget::keyPressEvent(event);
}

void CefWidget::UpdateSize() {
  CefBaseWidget::UpdateSize();
  if (browser_) browser_->GetHost()->NotifyMoveOrResizeStarted();
}

void CefWidget::FaviconDownloadCallback::OnDownloadImageFinished(
    const CefString& url, int, CefRefPtr<CefImage> image) {
  // TODO(cretz): should I somehow check if the page has changed before
  //  this came back?
  QIcon icon;
  if (image) {
    // TODO(cretz): This would be quicker if we used GetAsBitmap
    int width, height;
    auto png = image->GetAsPNG(1.0f, true, width, height);
    if (!png) {
      qDebug("Unable to get icon as PNG");
    } else {
      auto size = png->GetSize();
      std::vector<uchar> data(size);
      png->GetData(&data[0], size, 0);
      QPixmap pixmap;
      if (!pixmap.loadFromData(&data[0], (uint)size, "PNG")) {
        qDebug("Unable to convert icon as PNG");
      } else {
        icon.addPixmap(pixmap);
      }
    }
  }
  emit cef_widg_->FaviconChanged(QString::fromStdString(url.ToString()),
                                 icon);
}

bool CefWidget::NavEntryVisitor::Visit(CefRefPtr<CefNavigationEntry> entry,
                                       bool current,
                                       int, int) {
  NavEntry nav_entry = {
    QString::fromStdString(entry->GetURL().ToString()),
    QString::fromStdString(entry->GetTitle().ToString()),
    current
  };
  entries_.push_back(nav_entry);
  return true;
}

bool CefWidget::NavEntryCurrentSslVisitor::Visit(
    CefRefPtr<CefNavigationEntry> entry,
    bool current,
    int, int) {
  if (entry->IsValid() && current) ssl_status_ = entry->GetSSLStatus();
  return false;
}

void CefWidget::InitBrowser(const Bubble& bubble, const QString& url) {
  CefBrowserSettings settings;
  bubble.ApplyCefBrowserSettings(&settings);
  browser_ = CefBrowserHost::CreateBrowserSync(
        window_info_,
        handler_,
        CefString(url.toStdString()),
        settings,
        CefDictionaryValue::Create(),
        bubble.CreateCefRequestContext());
}

void CefWidget::AuthRequest(const QString& realm,
                            CefRefPtr<CefAuthCallback> callback) const {
  // Make a simple dialog to grab creds
  auto layout = new QGridLayout;
  auto header_label =
      new QLabel(QString("Authentication requested for: ") + realm);
  header_label->setTextFormat(Qt::PlainText);
  layout->addWidget(header_label, 0, 0, 1, 2, Qt::AlignCenter);
  layout->addWidget(new QLabel("Username:"), 1, 0);
  auto username = new QLineEdit;
  layout->addWidget(username, 1, 1);
  layout->setColumnStretch(1, 1);
  layout->addWidget(new QLabel("Password:"), 2, 0);
  auto password = new QLineEdit;
  password->setEchoMode(QLineEdit::Password);
  layout->addWidget(password, 2, 1);
  auto buttons = new QDialogButtonBox;
  buttons->addButton(QDialogButtonBox::Ok);
  buttons->addButton(QDialogButtonBox::Cancel);
  layout->addWidget(buttons, 3, 0, 1, 2, Qt::AlignCenter);
  auto dialog = new QDialog();
  dialog->setWindowTitle("Authentication Requested");
  dialog->setLayout(layout);
  connect(buttons->button(QDialogButtonBox::Ok), &QPushButton::clicked,
          dialog, &QDialog::accept);
  connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
          dialog, &QDialog::reject);

  if (dialog->exec() == QDialog::Accepted) {
    // TODO(cretz): downstream issue:
    //  https://bitbucket.org/chromiumembedded/cef/issues/2275
    if (username->text().isEmpty()) {
      QMessageBox::critical(dialog, "Empty Username",
                            "Empty usernames are not currently supported");
      // Defer a retry
      QTimer::singleShot(0, [=]() { AuthRequest(realm, callback); });
      return;
    }
    callback->Continue(CefString(username->text().toStdString()),
                       CefString(password->text().toStdString()));
  } else {
    callback->Cancel();
  }

  dialog->deleteLater();
}

}  // namespace doogie
