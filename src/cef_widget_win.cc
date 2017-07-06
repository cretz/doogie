#include "cef_widget.h"
#include <Windows.h>
#include "include/cef_client.h"

void CefWidget::InitBrowser() {
  CefWindowInfo win_info;
  win_info.SetAsChild((CefWindowHandle) winId(),
                      RECT { 0, 0, width(), height() });
  CefBrowserSettings settings;
  handler_ = CefRefPtr<CefHandler>(new CefHandler());
  browser_ = CefBrowserHost::CreateBrowserSync(win_info,
                                               handler_,
                                               CefString("http://example.com"),
                                               settings,
                                               nullptr);
}

void CefWidget::UpdateSize() {
  // Basically just set the browser handle to the same dimensions as widget
  if (browser_) {
    auto browser_host = browser_->GetHost();
    auto browser_win = browser_host->GetWindowHandle();
    SetWindowPos(browser_win, (HWND) this->winId(), 0, 0,
                 this->width(), this->height(),
                 SWP_NOZORDER);
    browser_host->NotifyMoveOrResizeStarted();
  }
}
