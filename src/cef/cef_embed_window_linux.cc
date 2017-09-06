#include "cef/cef_embed_window_linux.h"

namespace doogie {

CefEmbedWindow::CefEmbedWindow(QPointer<CefBaseWidget> cef_base_widget,
                               QWindow* parent)
    : QWindow(parent), cef_base_widget_(cef_base_widget) {}

void CefEmbedWindow::moveEvent(QMoveEvent*) {
  if (cef_base_widget_) cef_base_widget_->UpdateSize();
}

void CefEmbedWindow::resizeEvent(QResizeEvent*)  {
  if (cef_base_widget_) cef_base_widget_->UpdateSize();
}

}  // namespace doogie
