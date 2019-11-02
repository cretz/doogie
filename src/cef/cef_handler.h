#ifndef DOOGIE_CEF_HANDLER_H_
#define DOOGIE_CEF_HANDLER_H_

#include <QtWidgets>
#include <vector>

#include "cef/cef.h"
#include "download.h"

namespace doogie {

// Handler implementation for all common browser-level events. This mostly
// just calls signals, but the browser is not included so this handler is
// expected to be instantiated per browser instance.
class CefHandler :
    public QObject,
    public CefClient,
    public CefContextMenuHandler,
    public CefDisplayHandler,
    public CefDownloadHandler,
    public CefFindHandler,
    public CefFocusHandler,
    public CefJSDialogHandler,
    public CefKeyboardHandler,
    public CefLifeSpanHandler,
    public CefLoadHandler,
    public CefRequestHandler,
    public CefResourceRequestHandler {
  Q_OBJECT

 public:
  // Matches CEF's numbering, don't change
  enum WindowOpenType {
    OpenTypeUnknown,
    OpenTypeCurrentTab,
    OpenTypeSingletonTab,
    OpenTypeNewForegroundTab,
    OpenTypeNewBackgroundTab,
    OpenTypeNewPopup,
    OpenTypeNewWindow,
    OpenTypeSaveToDisk,
    OpenTypeOffTheRecord,
    OpenTypeIgnoreAction
  };
  Q_ENUM(WindowOpenType)

  CefHandler();

  CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override {
    return this;
  }

  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override {
    return this;
  }

  CefRefPtr<CefDownloadHandler> GetDownloadHandler() override {
    return this;
  }

  CefRefPtr<CefFindHandler> GetFindHandler() override {
    return this;
  }

  CefRefPtr<CefFocusHandler> GetFocusHandler() override {
    return this;
  }

  CefRefPtr<CefJSDialogHandler> GetJSDialogHandler() override {
    return this;
  }

  CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override {
    return this;
  }

  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
    return this;
  }

  CefRefPtr<CefLoadHandler> GetLoadHandler() override {
    return this;
  }

  CefRefPtr<CefRequestHandler> GetRequestHandler() override {
    return this;
  }

  // Context menu handler overrides...
  void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefContextMenuParams> params,
                           CefRefPtr<CefMenuModel> model) override;

  bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> frame,
                            CefRefPtr<CefContextMenuParams> params,
                            int command_id,
                            EventFlags event_flags) override;

  // Display handler overrides...
  void OnAddressChange(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       const CefString& url) override;
  void OnTitleChange(CefRefPtr<CefBrowser> browser,
                     const CefString& title) override;
  void OnStatusMessage(CefRefPtr<CefBrowser> browser,
                       const CefString& value) override;
  void OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
                          const std::vector<CefString>& icon_urls) override;
  void OnFullscreenModeChange(CefRefPtr<CefBrowser> browser,
                              bool fullscreen) override;

  // Download handler overrides...
  void OnBeforeDownload(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefDownloadItem> download_item,
      const CefString& suggested_name,
      CefRefPtr<CefBeforeDownloadCallback> callback) override;
  void OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefDownloadItem> download_item,
                         CefRefPtr<CefDownloadItemCallback> callback) override;

  // Find handler overrides...
  void OnFindResult(CefRefPtr<CefBrowser> browser,
                    int identifier,
                    int count,
                    const CefRect& selection_rect,
                    int active_match_ordinal,
                    bool final_update) override;

  // Focus handler overrides...
  typedef std::function<bool()> FocusAllowedCallback;
  void SetFocusAllowedCallback(FocusAllowedCallback callback) {
    focus_allowed_callback_ = callback;
  }
  void OnGotFocus(CefRefPtr<CefBrowser> browser) override;
  bool OnSetFocus(CefRefPtr<CefBrowser> browser, FocusSource source) override;
  void OnTakeFocus(CefRefPtr<CefBrowser> browser, bool next) override;

  // JS dialog handler overrides...
  typedef std::function<void(const QString&,
                             JSDialogType,
                             const QString&,
                             const QString&,
                             CefRefPtr<CefJSDialogCallback>,
                             bool&)> JsDialogCallback;
  bool OnJSDialog(CefRefPtr<CefBrowser> browser,
                  const CefString& origin_url,
                  JSDialogType dialog_type,
                  const CefString& message_text,
                  const CefString& default_prompt_text,
                  CefRefPtr<CefJSDialogCallback> callback,
                  bool& suppress_message) override;
  void SetJsDialogCallback(JsDialogCallback callback) {
    js_dialog_callback_ = callback;
  }
  bool OnBeforeUnloadDialog(CefRefPtr<CefBrowser> browser,
                            const CefString& message_text,
                            bool is_reload,
                            CefRefPtr<CefJSDialogCallback> callback) override;

  // Key handler overrides...
  typedef std::function<bool(const CefKeyEvent&,
                             CefEventHandle,
                             bool*)> PreKeyCallback;
  bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                     const CefKeyEvent& event,
                     CefEventHandle os_event,
                     bool* is_keyboard_shortcut) override;
  void SetPreKeyCallback(PreKeyCallback callback) {
    pre_key_callback_ = callback;
  }

  bool OnKeyEvent(CefRefPtr<CefBrowser> browser,
                  const CefKeyEvent& event,
                  CefEventHandle os_event) override;

  // Life span handler overrides...
  bool OnBeforePopup(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      const CefString& target_url,
      const CefString& target_frame_name,
      CefLifeSpanHandler::WindowOpenDisposition target_disposition,
      bool user_gesture,
      const CefPopupFeatures& popupFeatures,
      CefWindowInfo& windowInfo,
      CefRefPtr<CefClient>& client,
      CefBrowserSettings& settings,  // NOLINT(runtime/references)
      CefRefPtr<CefDictionaryValue>& extra_info,
      bool* no_javascript_access) override;
  bool DoClose(CefRefPtr<CefBrowser> browser) override;
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

  // Load handler overrides...
  void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                            bool is_loading,
                            bool can_go_back,
                            bool can_go_forward) override;
  void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                 CefRefPtr<CefFrame> frame,
                 int httpStatusCode) override;
  void OnLoadStart(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   TransitionType transition_type) override;
  void OnLoadError(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   ErrorCode errorCode,
                   const CefString& errorText,
                   const CefString& failedUrl) override;

  // Request handler overrides...
  bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                      CefRefPtr<CefFrame> frame,
                      CefRefPtr<CefRequest> request,
                      bool user_gesture,
                      bool is_redirect) override;
  bool OnCertificateError(CefRefPtr<CefBrowser> browser,
                          cef_errorcode_t cert_error,
                          const CefString& request_url,
                          CefRefPtr<CefSSLInfo> ssl_info,
                          CefRefPtr<CefRequestCallback> callback) override;
  bool OnOpenURLFromTab(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      const CefString& target_url,
      CefRequestHandler::WindowOpenDisposition target_disposition,
      bool user_gesture) override;
  CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      bool is_navigation,
      bool is_download,
      const CefString& request_initiator,
      bool& disable_default_handling) override;
  bool GetAuthCredentials(CefRefPtr<CefBrowser> browser,
                          const CefString& origin_url,
                          bool is_proxy,
                          const CefString& host,
                          int port,
                          const CefString& realm,
                          const CefString& scheme,
                          CefRefPtr<CefAuthCallback> callback) override;

  // Resource request handler overrides...
  typedef std::function<bool(
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request)> ResourceLoadCallback;
  void SetResourceLoadCallback(ResourceLoadCallback callback) {
    resource_load_callback_ = callback;
  }
  ReturnValue OnBeforeResourceLoad(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      CefRefPtr<CefRequestCallback> callback) override;

 signals:
  void PreContextMenu(CefRefPtr<CefContextMenuParams> params,
                      CefRefPtr<CefMenuModel> model);
  void ContextMenuCommand(CefRefPtr<CefContextMenuParams> params,
                          int command_id,
                          EventFlags event_flags);
  void UrlChanged(const QString& url);
  void TitleChanged(const QString& title);
  void StatusChanged(const QString& status);
  // Empty if no URL
  void FaviconUrlChanged(const QString& url);
  void FullscreenModeChange(bool fullscreen);
  void DownloadRequested(const Download& download,
                         CefRefPtr<CefBeforeDownloadCallback> callback);
  void DownloadUpdated(const Download& download);
  void FindResult(int identifier,
                  int count,
                  const CefRect& selection_rect,
                  int active_match_ordinal,
                  bool final_update);
  void FocusObtained();
  void ShowBeforeUnloadDialog(const QString& message_text,
                              bool is_reload,
                              CefRefPtr<CefJSDialogCallback> callback);
  void KeyEvent(const CefKeyEvent& event, CefEventHandle os_event);
  void Closed();
  void AfterCreated(CefRefPtr<CefBrowser> browser);
  void LoadStateChanged(bool is_loading,
                        bool can_go_back,
                        bool can_go_forward);
  void LoadEnd(CefRefPtr<CefFrame> frame,
               int httpStatusCode);
  void LoadStart(CefLoadHandler::TransitionType transition_type);
  void LoadError(CefRefPtr<CefFrame> frame,
                 ErrorCode error_code,
                 const QString& error_text,
                 const QString& failed_url);
  void CertificateError(
      cef_errorcode_t cert_error,
      const QString& request_url,
      CefRefPtr<CefSSLInfo> ssl_info,
      CefRefPtr<CefRequestCallback> callback);
  void PageOpen(WindowOpenType type, const QString& url, bool user_gesture);
  void AuthRequest(const QString& origin_url,
                   bool is_proxy,
                   const QString& host,
                   int port,
                   const QString& realm,
                   const QString& scheme,
                   CefRefPtr<CefAuthCallback> callback);
  void BrowserLog(const QString& str);

 private:
  bool load_start_js_no_op_to_create_context_ = true;
  bool popup_as_page_open_ = true;
  FocusAllowedCallback focus_allowed_callback_ = nullptr;
  JsDialogCallback js_dialog_callback_ = nullptr;
  PreKeyCallback pre_key_callback_ = nullptr;
  ResourceLoadCallback resource_load_callback_ = nullptr;

  IMPLEMENT_REFCOUNTING(CefHandler);
};

}  // namespace doogie

#endif  // DOOGIE_CEF_HANDLER_H_
