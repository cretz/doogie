#include "cef/cef_base_widget.h"

namespace doogie {

CefBaseWidget::CefBaseWidget(const Cef& cef, QWidget*  parent)
    : QWidget(parent), cef_(cef) {
  InitWindowInfo();
}

CefBaseWidget::~CefBaseWidget() {
}

const CefWindowInfo& CefBaseWidget::WindowInfo() const {
  return window_info_;
}

void CefBaseWidget::moveEvent(QMoveEvent* event) {
  this->UpdateSize();
  QWidget::moveEvent(event);
}

void CefBaseWidget::resizeEvent(QResizeEvent* event) {
  this->UpdateSize();
  QWidget::resizeEvent(event);
}

}  // namespace doogie
