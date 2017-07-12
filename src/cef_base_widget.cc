#include "cef_base_widget.h"

CefBaseWidget::CefBaseWidget(Cef *cef, QWidget *parent)
    : QWidget(parent), cef_(cef) {
  InitWindowInfo();
}

CefBaseWidget::~CefBaseWidget() {

}

void CefBaseWidget::moveEvent(QMoveEvent *) {
  this->UpdateSize();
}

void CefBaseWidget::resizeEvent(QResizeEvent *) {
  this->UpdateSize();
}

void CefBaseWidget::UpdateSize() {

}
