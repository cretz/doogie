#ifndef DOOGIE_CEF_HANDLER_H_
#define DOOGIE_CEF_HANDLER_H_

#include <QtWidgets>
#include "cef.h"

namespace doogie {

class CefHandler :
    public QObject,
    public CefClient,
    public CefDisplayHandler,
    public CefFindHandler,
    public CefFocusHandler,
    public CefKeyboardHandler,
    public CefLifeSpanHandler,
    public CefLoadHandler,
    public CefRequestHandler {
  Q_OBJECT

 public:
  CefHandler();

  virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override {
    return this;
  }

  virtual CefRefPtr<CefFindHandler> GetFindHandler() override {
    return this;
  }

  virtual CefRefPtr<CefFocusHandler> GetFocusHandler() override {
    return this;
  }

  virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override {
    return this;
  }

  virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
    return this;
  }

  virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override {
    return this;
  }

  virtual CefRefPtr<CefRequestHandler> GetRequestHandler() override {
    return this;
  }

  // Display handler overrides...
  virtual void OnAddressChange(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               const CefString& url) override;

  virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
                             const CefString& title) override;

  virtual void OnStatusMessage(CefRefPtr<CefBrowser> browser,
                               const CefString& value) override;
  virtual void OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
                                  const std::vector<CefString>& icon_urls) override;

  // Find handler overrides...
  virtual void OnFindResult(CefRefPtr<CefBrowser> browser,
                            int identifier,
                            int count,
                            const CefRect& selection_rect,
                            int active_match_ordinal,
                            bool final_update);

  // Focus handler overrides...
  virtual void OnGotFocus(CefRefPtr<CefBrowser> browser) override;

  // Key handler overrides...
  virtual bool OnKeyEvent(CefRefPtr<CefBrowser> browser,
                          const CefKeyEvent& event,
                          CefEventHandle os_event) override;

  // Life span handler overrides...
  virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;

  virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

  // Load handler overrides...
  virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                    bool is_loading,
                                    bool can_go_back,
                                    bool can_go_forward) override;
  virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) override;

  // Request handler overrides...
  virtual bool OnOpenURLFromTab(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                const CefString& target_url,
                                CefRequestHandler::WindowOpenDisposition target_disposition,
                                bool user_gesture) override;

 signals:
  void UrlChanged(const QString& url);
  void TitleChanged(const QString& title);
  void StatusChanged(const QString& status);
  // Empty if no URL
  void FaviconUrlChanged(const QString& url);
  void FindResult(int identifier,
                  int count,
                  const CefRect& selection_rect,
                  int active_match_ordinal,
                  bool final_update);
  void FocusObtained();
  void KeyEvent(const CefKeyEvent& event, CefEventHandle os_event);
  void Closed();
  void AfterCreated(CefRefPtr<CefBrowser> browser);
  void LoadStateChanged(bool is_loading,
                        bool can_go_back,
                        bool can_go_forward);
  void LoadEnd(CefRefPtr<CefFrame> frame,
               int httpStatusCode);
  void PageOpen(CefRequestHandler::WindowOpenDisposition type,
                const QString& url,
                bool user_gesture);

 private:
  IMPLEMENT_REFCOUNTING(CefHandler)
};

}  // namespace doogie

#endif  // DOOGIE_CEF_HANDLER_H_
