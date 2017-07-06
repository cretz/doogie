#include "cef_widget.h"

CefWidget::CefWidget(Cef *cef, QWidget *parent) : QWidget(parent), cef_(cef) {
  InitBrowser();

  connect(handler_, &CefHandler::UrlChanged, this, &CefWidget::UrlChanged);
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
