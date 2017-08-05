#include "cef_embed_window_linux.h"

namespace doogie {

CefEmbedWindow::CefEmbedWindow(QPointer<CefWidget> cef_widget,
                               QWindow* parent)
    : QWindow(parent), cef_widget_(cef_widget) {}

void CefEmbedWindow::moveEvent(QMoveEvent*) {
  if (cef_widget_) cef_widget_->UpdateSize();
}

void CefEmbedWindow::resizeEvent(QResizeEvent*)  {
  if (cef_widget_) cef_widget_->UpdateSize();
}

}  // namespace doogie
