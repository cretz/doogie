#ifndef DOOGIE_CEFHANDLER_H_
#define DOOGIE_CEFHANDLER_H_

#include <QtWidgets>
#include "include/cef_client.h"

class CefHandler :
    public QObject,
    public CefClient,
    public CefDisplayHandler,
    public CefFocusHandler {
  Q_OBJECT
 public:
  CefHandler();

  virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override {
    return this;
  }

  virtual CefRefPtr<CefFocusHandler> GetFocusHandler() override {
    return this;
  }

  virtual void OnAddressChange(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               const CefString& url) override;

 private:
  IMPLEMENT_REFCOUNTING(CefHandler);

 signals:
  void UrlChanged(const QString &url);
};

#endif // DOOGIE_CEFHANDLER_H_
