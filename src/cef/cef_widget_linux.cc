#include "cef/cef_widget.h"

#include <X11/Xlib.h>

#include "cef/cef_handler.h"
#include "cef/cef_embed_window_linux.h"

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
  if (browser_) {
    auto browser_host = browser_->GetHost();
    auto browser_win = browser_host->GetWindowHandle();
    auto xdisplay = cef_get_xdisplay();
    XWindowChanges changes = {};
    changes.x = 0;
    changes.y = 0;
    changes.width = width();
    changes.height = height();
    XConfigureWindow(xdisplay,
                     browser_win,
                     CWX | CWY | CWHeight | CWWidth,
                     &changes);
    browser_host->NotifyMoveOrResizeStarted();
  }
}

}  // namespace doogie
