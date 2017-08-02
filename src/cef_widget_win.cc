#include "cef_widget.h"
#include <Windows.h>
#include "profile.h"
#include "bubble.h"

namespace doogie {

void CefWidget::InitBrowser(const QString& url) {
  browser_ = CefBrowserHost::CreateBrowserSync(
        window_info_,
        handler_,
        CefString(url.toStdString()),
        bubble_->CreateCefBrowserSettings(),
        nullptr);
}

void CefWidget::UpdateSize() {
  CefBaseWidget::UpdateSize();
  if (browser_) browser_->GetHost()->NotifyMoveOrResizeStarted();
}

}  // namespace doogie
