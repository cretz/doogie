#include "cef_widget.h"

CefWidget::CefWidget(Cef *cef, const QString &url, QWidget *parent)
    : QWidget(parent), cef_(cef) {
  handler_ = CefRefPtr<CefHandler>(new CefHandler);
  connect(handler_, &CefHandler::UrlChanged,
          this, &CefWidget::UrlChanged);
  connect(handler_, &CefHandler::TitleChanged,
          this, &CefWidget::TitleChanged);
  connect(handler_, &CefHandler::StatusChanged,
          this, &CefWidget::StatusChanged);
  connect(handler_, &CefHandler::FaviconUrlChanged,
          [this](const QString &url) {
    static const auto favicon_sig =
        QMetaMethod::fromSignal(&CefWidget::FaviconChanged);
    if (this->isSignalConnected(favicon_sig) && browser_) {
      // Download the favicon
      browser_->GetHost()->DownloadImage(CefString(url.toStdString()),
                                         true,
                                         16,
                                         false,
                                         new CefWidget::FaviconDownloadCallback(this));
    }
  });
  connect(handler_, &CefHandler::LoadStateChanged,
          this, &CefWidget::LoadStateChanged);
  connect(handler_, &CefHandler::PageOpen,
          this, &CefWidget::PageOpen);

  InitBrowser(url);
}

CefWidget::~CefWidget() {
  if (browser_) {
    browser_->GetHost()->CloseBrowser(true);
  }
}

QPointer<QWidget> CefWidget::OverrideWidget() {
  return override_widget_;
}

void CefWidget::LoadUrl(const QString &url) {
  if (browser_) {
    browser_->GetMainFrame()->LoadURL(CefString(url.toStdString()));
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
    if (ignore_cache) browser_->ReloadIgnoreCache();
    else browser_->Reload();
  }
}

void CefWidget::Stop() {
  if (browser_) {
    browser_->StopLoad();
  }
}

std::vector<CefWidget::NavEntry> CefWidget::NavEntries() {
  CefRefPtr<CefWidget::NavEntryVisitor> visitor = new CefWidget::NavEntryVisitor;
  if (browser_) {
    browser_->GetHost()->GetNavigationEntries(visitor, false);
  }
  return visitor->Entries();
}

void CefWidget::focusInEvent(QFocusEvent *event) {
  QWidget::focusInEvent(event);
  if (browser_) {
    browser_->GetHost()->SetFocus(true);
  }
}

void CefWidget::focusOutEvent(QFocusEvent *event) {
  QWidget::focusOutEvent(event);
  if (browser_) {
    browser_->GetHost()->SetFocus(false);
  }
}

void CefWidget::moveEvent(QMoveEvent *) {
  this->UpdateSize();
}

void CefWidget::resizeEvent(QResizeEvent *) {
  this->UpdateSize();
}

void CefWidget::FaviconDownloadCallback::OnDownloadImageFinished(
    const CefString &,
    int,
    CefRefPtr<CefImage> image) {
  // TODO: should I somehow check if the page has changed before this came back?
  QIcon icon;
  if (image) {
    // TODO: This would be quicker if we used GetAsBitmap
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
  emit cef_widg_->FaviconChanged(icon);
}
