#include "cef_handler.h"

CefHandler::CefHandler() {}

void CefHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 const CefString &url) {
  if (frame->IsMain()) emit UrlChanged(QString::fromStdString(url.ToString()));
}
