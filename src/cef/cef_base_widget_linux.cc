#include "cef/cef_base_widget.h"

#include "cef/cef_embed_window_linux.h"

namespace doogie {

void CefBaseWidget::InitWindowInfo() {
    auto win = new CefEmbedWindow(this);
    window_info_.SetAsChild((CefWindowHandle) win->winId(),
                            CefRect(0, 0, width(), height()));
    override_widget_ = QWidget::createWindowContainer(win, this);
}

void CefBaseWidget::ForwardKeyboardEventsFrom(CefRefPtr<CefHandler>) {
  // TODO(cretz): this...
}

void CefBaseWidget::UpdateSize() {
}

}  // namespace doogie
