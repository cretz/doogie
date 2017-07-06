#include "cef_widget.h"
#include <X11/Xlib.h>
#include "include/cef_client.h"
#include "cef_handler.h"
#include "cef_embed_window_linux.h"

QPointer<QWidget> CefWidget::EmbedBrowser(QMainWindow *main_win,
                                          QLineEdit *url_line_edit) {
  CefWindowInfo win_info;
  auto win = new CefEmbedWindow(this);
  win_info.SetAsChild((CefWindowHandle) win->winId(),
                      CefRect(0, 0, width(), height()));
  CefBrowserSettings settings;
  CefRefPtr<CefHandler> handler(new CefHandler(main_win,
                                               url_line_edit,
                                               this));
  browser_ = CefBrowserHost::CreateBrowserSync(win_info,
                                               handler,
                                               CefString("http://example.com"),
                                               settings,
                                               nullptr);
  return QWidget::createWindowContainer(win, main_win);
}


void CefWidget::UpdateSize() {
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
