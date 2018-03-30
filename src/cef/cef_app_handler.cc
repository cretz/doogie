#include "cef/cef_app_handler.h"

#include "cosmetic_blocker.h"

namespace doogie {

CefAppHandler::CefAppHandler() {}

void CefAppHandler::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context) {
  // Prevent document.webkitExitFullscreen from being mutated because
  //  we need to call it internally in Doogie.
  // TODO(cretz): Find a better way, ref:
  //  http://www.magpcss.org/ceforum/viewtopic.php?f=6&t=13642#p35988
  frame->ExecuteJavaScript(
      "Object.defineProperty(document, 'webkitExitFullscreen', { "
      "  value: document.webkitExitFullscreen, "
      "  writable: false "
      "});",
      "<doogie>",
      0);

  blocker_.OnFrameCreated(browser, frame, context);
}

void CefAppHandler::OnWebKitInitialized() {
}

}  // namespace doogie
