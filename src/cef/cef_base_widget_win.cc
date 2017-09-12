#include "cef/cef_base_widget.h"

#include <Windows.h>

namespace doogie {

void CefBaseWidget::InitWindowInfo() {
  window_info_.SetAsChild(reinterpret_cast<CefWindowHandle>(winId()),
                          RECT { 0, 0, width(), height() });
  window_info_.ex_style = WS_EX_NOACTIVATE;
}

void CefBaseWidget::ForwardKeyboardEventsFrom(CefRefPtr<CefHandler> handler) {
  // Note, we used to handle CefHandler::KeyEvent signal, but this is processed
  //  *after* JS which means the page can prevent it. Here we now properly
  //  handle it before JS gets it. The problem is that Qt doesn't tell us
  //  whether the native event was handled so we can't tell CEF. In theory we
  //  should be able to translate the native key event to a Qt event and call
  //  keyPressEvent/keyReleaseEvent and check event::accepted but the
  //  translation is non-trivial and code reuse is behind Qt private ifaces.
  auto win_id = reinterpret_cast<HWND>(winId());
  handler->SetPreKeyCallback([=](const CefKeyEvent& event,
                                 CefEventHandle os_event,
                                 bool*) {
    if (os_event && IsForwardableKeyEvent(event)) {
      PostMessage(win_id, os_event->message,
                  os_event->wParam, os_event->lParam);
    }
    return false;
  });
}

void CefBaseWidget::UpdateSize() {
  // Basically just set the cef handle to the same dimensions as widget
  auto my_win = reinterpret_cast<HWND>(winId());
  auto cef_win = GetWindow(my_win, GW_CHILD);
  SetWindowPos(cef_win, my_win, 0, 0, width(), height(), SWP_NOZORDER);
}

}  // namespace doogie
