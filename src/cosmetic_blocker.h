#ifndef DOOGIE_COSMETIC_BLOCKER_H_
#define DOOGIE_COSMETIC_BLOCKER_H_

#include <QtWidgets>

#include "cef/cef_base.h"

namespace doogie {

// Blocker class to be called for element hiding.
class CosmeticBlocker {
 public:
  void OnFrameCreated(CefRefPtr<CefBrowser> browser,
                      CefRefPtr<CefFrame> frame,
                      CefRefPtr<CefV8Context> context);

 private:
  class MutationCallback : public CefV8Handler {
   public:
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override;

   private:
    IMPLEMENT_REFCOUNTING(MutationCallback);
  };
};

}  // namespace doogie

#endif  // DOOGIE_COSMETIC_BLOCKER_H_
