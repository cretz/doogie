#include "cef/cef_handler.h"

namespace doogie {

CefHandler::CefHandler() {
  qRegisterMetaType<WindowOpenType>("WindowOpenType");
}

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

void CefHandler::OnBeforeDownload(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    const CefString& suggested_file_name,
    CefRefPtr<CefBeforeDownloadCallback> callback) {
  emit DownloadRequested(
        Download(download_item,
                 QString::fromStdString(suggested_file_name.ToString())),
        callback);
}

void CefHandler::OnDownloadUpdated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    CefRefPtr<CefDownloadItemCallback> callback) {
  emit DownloadUpdated(Download(download_item, callback));
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

bool CefHandler::OnSetFocus(CefRefPtr<CefBrowser>, FocusSource) {
  return false;
}

void CefHandler::OnTakeFocus(CefRefPtr<CefBrowser>, bool) {
}

bool CefHandler::OnJSDialog(CefRefPtr<CefBrowser> browser,
                            const CefString& origin_url,
                            JSDialogType dialog_type,
                            const CefString& message_text,
                            const CefString& default_prompt_text,
                            CefRefPtr<CefJSDialogCallback> callback,
                            bool& suppress_message) {
  if (js_dialog_callback_) {
    js_dialog_callback_(QString::fromStdString(origin_url.ToString()),
                        dialog_type,
                        QString::fromStdString(message_text.ToString()),
                        QString::fromStdString(default_prompt_text.ToString()),
                        callback,
                        suppress_message);
    return true;
  } else {
    return false;
  }
}

void CefHandler::SetJsDialogCallback(JsDialogCallback callback) {
  js_dialog_callback_ = callback;
}

bool CefHandler::OnBeforeUnloadDialog(CefRefPtr<CefBrowser> browser,
                                      const CefString& message_text,
                                      bool is_reload,
                                      CefRefPtr<CefJSDialogCallback> callback) {
  emit ShowBeforeUnloadDialog(QString::fromStdString(message_text.ToString()),
                              is_reload,
                              callback);
  return true;
}

bool CefHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                            const CefKeyEvent& event,
                            CefEventHandle os_event) {
  emit KeyEvent(event, os_event);
  // Let's set true to say we handled it
  return true;
}

bool CefHandler::OnBeforePopup(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& target_url,
    const CefString&,
    CefLifeSpanHandler::WindowOpenDisposition target_disposition,
    bool user_gesture,
    const CefPopupFeatures&,
    CefWindowInfo&,
    CefRefPtr<CefClient>&,
    CefBrowserSettings&,
    bool*) {
  if (popup_as_page_open_) {
    emit PageOpen(static_cast<WindowOpenType>(target_disposition),
                  QString::fromStdString(target_url.ToString()),
                  user_gesture);
    return true;
  }
  return false;
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

void CefHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             TransitionType transition_type) {
  // If we have JS we want to execute before anything else on this page,
  //  we do it in CefRenderProcessHandler::OnContextCreated. But that is
  //  not called on pages without JS, so we do a little no-op JS code here
  //  to trigger JS context creation.
  if (load_start_js_no_op_to_create_context_) {
    frame->ExecuteJavaScript("// no-op", "<doogie>", 0);
  }
  if (frame->IsMain()) emit LoadStart(transition_type);
}

void CefHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             ErrorCode error_code,
                             const CefString& error_text,
                             const CefString& failed_url) {
  emit LoadError(frame, error_code,
                 QString::fromStdString(error_text.ToString()),
                 QString::fromStdString(failed_url.ToString()));
}

bool CefHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefRequest> request,
                                bool) {
  return false;
}

bool CefHandler::OnCertificateError(CefRefPtr<CefBrowser> browser,
                                    cef_errorcode_t cert_error,
                                    const CefString& request_url,
                                    CefRefPtr<CefSSLInfo> ssl_info,
                                    CefRefPtr<CefRequestCallback> callback) {
  emit CertificateError(cert_error,
                        QString::fromStdString(request_url.ToString()),
                        ssl_info, callback);
  return true;
}

bool CefHandler::OnOpenURLFromTab(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& target_url,
    CefRequestHandler::WindowOpenDisposition target_disposition,
    bool user_gesture) {
  emit PageOpen(static_cast<WindowOpenType>(target_disposition),
               QString::fromStdString(target_url.ToString()),
               user_gesture);
  return true;
}

}  // namespace doogie
