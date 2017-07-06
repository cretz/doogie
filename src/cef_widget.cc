#include "cef_widget.h"

CefWidget::CefWidget(Cef *cef, QWidget *parent) : QWidget(parent), cef_(cef) {}

CefWidget::~CefWidget() {
  if (browser_) {
    browser_->GetHost()->CloseBrowser(true);
  }
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
