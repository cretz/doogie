#include "cef_base_widget.h"
#include <Windows.h>

void CefBaseWidget::InitWindowInfo() {
  window_info_.SetAsChild((CefWindowHandle) winId(),
                          RECT { 0, 0, width(), height() });
}
