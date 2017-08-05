#include "cef/cef_app_handler.h"

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

}  // namespace doogie
