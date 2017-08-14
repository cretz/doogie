#include "blocker.h"

namespace doogie {

void Blocker::Init() {
  // For element hiding we are going to walk the nodes and display: none
  // all of them
  auto code = CefString(
      "var doogie = {}; "
      "native function mutationCallback(); "
      "Object.defineProperty(doogie, 'mutationCallback', { "
      "  value: mutationCallback, "
      "  writable: false "
      "});");
  CefRegisterExtension("doogie/blocker",
                       code,
                       new Blocker::MutationCallback());
}

void Blocker::OnFrameCreated(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefRefPtr<CefV8Context> context) {
  // TODO(cretz): waiting on http://www.magpcss.org/ceforum/viewtopic.php?f=6&t=15360
  /*
  frame->ExecuteJavaScript(
      "const obs = new MutationObserver(doogie.mutationCallback); "
      "obs.observe(document, { childList: true, subtree: true }); ",
      "<doogie>",
      0
  );
  */
}

bool Blocker::MutationCallback::Execute(const CefString& name,
                                        CefRefPtr<CefV8Value> object,
                                        const CefV8ValueList& arguments,
                                        CefRefPtr<CefV8Value>& retval,
                                        CefString& exception) {
  if (name != "mutationCallback" || arguments.empty()) return false;
  auto arg = arguments[0];
  if (!arg->IsArray()) return false;
  /*
  qDebug() << "!!Changed " << arg->GetArrayLength() << " nodes";
  for (int i = 0; i < arg->GetArrayLength(); i++) {
    auto record = arg->GetValue(i);
    auto target = record->GetValue("target");
    if (target) {
      qDebug() << "!!Target: " << QString::fromStdString(target->GetValue("nodeName")->GetStringValue().ToString());
    }
    auto added = record->GetValue("addedNodes");
    if (added) {
      auto len = added->GetValue("length")->GetIntValue();
      qDebug() << "!!Added node count: " << len;
      for (int j = 0; j < len; j++) {
        auto item = added->GetValue(j);
        if (item) {
          qDebug() << "!!Type: " << item->GetValue("nodeType")->GetIntValue();
          qDebug() << "!!Name: " << QString::fromStdString(item->GetValue("nodeName")->GetStringValue().ToString());
        }
      }
    }
  }
  */
  return true;
}

}  // namespace doogie
