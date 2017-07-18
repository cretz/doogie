#include "cef_handler.h"

namespace doogie {

CefHandler::CefHandler() {}

void CefHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) {
  emit PreContextMenu(params, model);
}

bool CefHandler::OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefContextMenuParams> params,
                                      int command_id,
                                      EventFlags event_flags) {
  if (command_id < MENU_ID_USER_FIRST) return false;
  emit ContextMenuCommand(params, command_id, event_flags);
  return true;
}

void CefHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 const CefString& url) {
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
                                    const std::vector<CefString>& icon_urls) {
  if (!icon_urls.empty()) {
    emit FaviconUrlChanged(QString::fromStdString(icon_urls[0].ToString()));
  }
}

void CefHandler::OnFindResult(CefRefPtr<CefBrowser> browser,
                              int identifier,
                              int count,
                              const CefRect& selection_rect,
                              int active_match_ordinal,
                              bool final_update) {
  emit FindResult(identifier, count, selection_rect,
                  active_match_ordinal, final_update);
}

void CefHandler::OnGotFocus(CefRefPtr<CefBrowser> browser) {
  emit FocusObtained();
}

bool CefHandler::OnSetFocus(CefRefPtr<CefBrowser> browser,
                            FocusSource source) {
  return false;
}

bool CefHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                            const CefKeyEvent& event,
                            CefEventHandle os_event) {
  emit KeyEvent(event, os_event);
  // Let's set true to say we handled it
  return true;
}

bool CefHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  emit Closed();
  // Per the docs, we want to return tru here to prevent CEF from
  // closing our window.
  return true;
}

void CefHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  emit AfterCreated(browser);
}

void CefHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                      bool is_loading,
                                      bool can_go_back,
                                      bool can_go_forward) {
  emit LoadStateChanged(is_loading, can_go_back, can_go_forward);
}

void CefHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           int http_status_code) {
  emit LoadEnd(frame, http_status_code);
}

bool CefHandler::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  const CefString& target_url,
                                  CefRequestHandler::WindowOpenDisposition target_disposition,  // NOLINT(whitespace/line_length)
                                  bool user_gesture) {
  emit PageOpen(target_disposition,
               QString::fromStdString(target_url.ToString()),
               user_gesture);
  return true;
}

}  // namespace doogie
