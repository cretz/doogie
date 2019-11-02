#ifndef DOOGIE_CEF_APP_HANDLER_H_
#define DOOGIE_CEF_APP_HANDLER_H_

#include <QtWidgets>

#include "cosmetic_blocker.h"
#include "cef/cef_base.h"

namespace doogie {

// Handler for overall app features.
class CefAppHandler :
    public QObject,
    public CefApp,
    public CefBrowserProcessHandler,
    public CefRenderProcessHandler {
  Q_OBJECT

 public:
  CefAppHandler();

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
    return this;
  }

  // Render process handler overrides...
  void OnContextCreated(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefV8Context> context) override;

  void OnWebKitInitialized() override;

 private:
  CosmeticBlocker blocker_;

  IMPLEMENT_REFCOUNTING(CefAppHandler);
};

}  // namespace doogie

#endif  // DOOGIE_CEF_APP_HANDLER_H_
