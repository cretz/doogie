#include "cef_base_widget.h"
#include <Windows.h>

void CefBaseWidget::InitWindowInfo() {
  window_info_.SetAsChild((CefWindowHandle) winId(),
                          RECT { 0, 0, width(), height() });
}

void CefBaseWidget::UpdateSize() {
  // Basically just set the cef handle to the same dimensions as widget
  auto my_win = (HWND) this->winId();
  auto cef_win = GetWindow((HWND) this->winId(), GW_CHILD);
  SetWindowPos(cef_win, my_win, 0, 0,
               this->width(), this->height(),
               SWP_NOZORDER);
}
