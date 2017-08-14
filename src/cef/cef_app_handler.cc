#include "cef/cef_app_handler.h"

#include "blocker.h"

namespace doogie {

CefAppHandler::CefAppHandler() {}

bool CefAppHandler::OnBeforeNavigation(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRenderProcessHandler::NavigationType navigation_type,
    bool is_redirect) {
  if (before_nav_callback_) {
    return before_nav_callback_(browser, frame, request,
                                navigation_type, is_redirect);
  }
  return false;
}

void CefAppHandler::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context) {
  blocker_.OnFrameCreated(browser, frame, context);
}

void CefAppHandler::OnWebKitInitialized() {
  blocker_.Init();
}

}  // namespace doogie
