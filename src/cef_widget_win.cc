#include "cef_widget.h"
#include <Windows.h>

namespace doogie {

void CefWidget::InitBrowser(const QString& url) {
  CefBrowserSettings settings;
#ifdef QT_DEBUG
  settings.web_security = STATE_DISABLED;
#endif
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

}  // namespace doogie
