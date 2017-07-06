#ifndef DOOGIE_CEFHANDLER_H_
#define DOOGIE_CEFHANDLER_H_

#include <QtWidgets>
#include "include/cef_client.h"

class CefHandler :
    public CefClient,
    public CefDisplayHandler,
    public CefFocusHandler {
 public:
  explicit CefHandler(QPointer<QMainWindow> main_win,
                      QPointer<QLineEdit> url_line_edit,
                      QPointer<QWidget> browser_widg);

  virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override {
    return this;
  }

  virtual CefRefPtr<CefFocusHandler> GetFocusHandler() override {
    return this;
  }

  virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
                             const CefString& title) override;

  virtual void OnAddressChange(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               const CefString& url) override;

  virtual void OnGotFocus(CefRefPtr<CefBrowser> browser) override;

 private:
  QPointer<QMainWindow> main_win_;
  QPointer<QLineEdit> url_line_edit_;
  QPointer<QWidget> browser_widg_;

  IMPLEMENT_REFCOUNTING(CefHandler);
};

#endif // DOOGIE_CEFHANDLER_H_
