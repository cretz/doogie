#include "cef_widget.h"
#include <Windows.h>

void CefWidget::InitBrowser(const QString &url) {
  CefBrowserSettings settings;
  browser_ = CefBrowserHost::CreateBrowserSync(window_info_,
                                               handler_,
                                               CefString(url.toStdString()),
                                               settings,
                                               nullptr);
}

void CefWidget::UpdateSize() {
  CefBaseWidget::UpdateSize();
  if (browser_) browser_->GetHost()->NotifyMoveOrResizeStarted();
}
