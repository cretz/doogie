#include "cef_handler.h"

CefHandler::CefHandler() {}

void CefHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 const CefString &url) {
  if (frame->IsMain()) emit UrlChanged(QString::fromStdString(url.ToString()));
}

void CefHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                               const CefString& title) {
  emit TitleChanged(QString::fromStdString(title.ToString()));
}

void CefHandler::OnStatusMessage(CefRefPtr<CefBrowser> browser,
                                 const CefString& value) {
  emit StatusChanged(QString::fromStdString(value.ToString()));
}

void CefHandler::OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
                                    const std::vector<CefString> &icon_urls) {
  if (!icon_urls.empty()) {
    emit FaviconUrlChanged(QString::fromStdString(icon_urls[0].ToString()));
  }
}

void CefHandler::OnGotFocus(CefRefPtr<CefBrowser> browser) {
  emit FocusObtained();
}

void CefHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                      bool is_loading,
                                      bool can_go_back,
                                      bool can_go_forward) {
  emit LoadStateChanged(is_loading, can_go_back, can_go_forward);
}

bool CefHandler::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  const CefString &target_url,
                                  CefRequestHandler::WindowOpenDisposition target_disposition,
                                  bool user_gesture) {
  emit PageOpen(target_disposition,
               QString::fromStdString(target_url.ToString()),
               user_gesture);
  return true;
}
