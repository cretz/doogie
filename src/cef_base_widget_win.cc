#include "cef_base_widget.h"
#include <Windows.h>

namespace doogie {

void CefBaseWidget::InitWindowInfo() {
  window_info_.SetAsChild(reinterpret_cast<CefWindowHandle>(winId()),
                          RECT { 0, 0, width(), height() });
}

void CefBaseWidget::ForwardKeyboardEventsFrom(CefRefPtr<CefHandler> handler) {
  // This handler doesn't get removed properly if we don't pass "this"
  //  as the context
  connect(handler, &CefHandler::KeyEvent, this,
          [this](const CefKeyEvent& event, CefEventHandle os_event) {
    // We have to handle Alt+F4 ourselves TODO: right?
    if (event.is_system_key && event.modifiers & EVENTFLAG_ALT_DOWN &&
        event.windows_key_code == VK_F4) {
      window()->close();
    } else if (os_event) {
      PostMessage(reinterpret_cast<HWND>(winId()), os_event->message,
                  os_event->wParam, os_event->lParam);
    }
  });
}

void CefBaseWidget::UpdateSize() {
  // Basically just set the cef handle to the same dimensions as widget
  auto my_win = reinterpret_cast<HWND>(winId());
  auto cef_win = GetWindow(my_win, GW_CHILD);
  SetWindowPos(cef_win, my_win, 0, 0, width(), height(), SWP_NOZORDER);
}

}  // namespace doogie
