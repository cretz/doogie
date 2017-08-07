#include "cef/cef_widget.h"

#include <Windows.h>

#include "bubble.h"
#include "profile.h"

namespace doogie {

void CefWidget::InitBrowser(const Bubble& bubble, const QString& url) {
  CefBrowserSettings settings;
  bubble.ApplyCefBrowserSettings(&settings);
  browser_ = CefBrowserHost::CreateBrowserSync(
        window_info_,
        handler_,
        CefString(url.toStdString()),
        settings,
        bubble.CreateCefRequestContext());
}

void CefWidget::UpdateSize() {
  CefBaseWidget::UpdateSize();
  if (browser_) browser_->GetHost()->NotifyMoveOrResizeStarted();
}

}  // namespace doogie
