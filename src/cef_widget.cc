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
  connect(handler_, &CefHandler::TabOpen,
          this, &CefWidget::TabOpen);

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

void CefWidget::moveEvent(QMoveEvent *event) {
  this->UpdateSize();
}

void CefWidget::resizeEvent(QResizeEvent *event) {
  this->UpdateSize();
}

void CefWidget::FaviconDownloadCallback::OnDownloadImageFinished(
    const CefString &image_url,
    int http_status_code,
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
      if (!pixmap.loadFromData(&data[0], size, "PNG")) {
        qDebug("Unable to convert icon as PNG");
      } else {
        icon.addPixmap(pixmap);
      }
    }
  }
  emit cef_widg_->FaviconChanged(icon);
}
