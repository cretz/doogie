#include "cosmetic_blocker.h"

namespace doogie {

void CosmeticBlocker::OnFrameCreated(CefRefPtr<CefBrowser>,
                             CefRefPtr<CefFrame> frame,
                             CefRefPtr<CefV8Context> context) {
  // We are going to create a mutation callback function, use it,
  //  then remove it to prevent fingerprinting
  auto global = context->GetGlobal();
  global->SetValue(
      "mutationCallback",
      CefV8Value::CreateFunction("mutationCallback",
                                 new CosmeticBlocker::MutationCallback()),
      V8_PROPERTY_ATTRIBUTE_NONE);
  frame->ExecuteJavaScript(
      "const obs = new MutationObserver(mutationCallback);\n"
      "obs.observe(document, { childList: true, subtree: true });\n"
      "delete window.mutationCallback;",
      "<doogie>",
      0);
}

bool CosmeticBlocker::MutationCallback::Execute(const CefString& name,
                                        CefRefPtr<CefV8Value> /*object*/,
                                        const CefV8ValueList& arguments,
                                        CefRefPtr<CefV8Value>& /*retval*/,
                                        CefString& /*exception*/) {
  if (name != "mutationCallback" || arguments.empty()) return false;
  auto arg = arguments[0];
  if (!arg->IsArray()) return false;
  // TODO(cretz): The rest of this for cosmetic filters
  // qDebug() << "!!Changed " << arg->GetArrayLength() << " nodes";
  return true;
}

}  // namespace doogie
