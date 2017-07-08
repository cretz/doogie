#ifndef DOOGIE_CEFHANDLER_H_
#define DOOGIE_CEFHANDLER_H_

#include <QtWidgets>
#include "cef.h"

class CefHandler :
    public QObject,
    public CefClient,
    public CefDisplayHandler,
    public CefFocusHandler,
    public CefRequestHandler {
  Q_OBJECT
 public:
  CefHandler();

  virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override {
    return this;
  }

  virtual CefRefPtr<CefFocusHandler> GetFocusHandler() override {
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

  // Request handler overrides...
  virtual bool OnOpenURLFromTab(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                const CefString& target_url,
                                CefRequestHandler::WindowOpenDisposition target_disposition,
                                bool user_gesture) override;
 private:
  IMPLEMENT_REFCOUNTING(CefHandler);

 signals:
  void UrlChanged(const QString &url);
  void TitleChanged(const QString &title);
  void StatusChanged(const QString &status);
  // Empty if no URL
  void FaviconUrlChanged(const QString &url);
  void TabOpen(CefRequestHandler::WindowOpenDisposition type,
               const QString &url,
               bool user_gesture);
};

#endif // DOOGIE_CEFHANDLER_H_
