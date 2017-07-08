#ifndef DOOGIE_CEFWIDGET_H_
#define DOOGIE_CEFWIDGET_H_

#include <QtWidgets>
#include "cef.h"
#include "cef_handler.h"

class CefWidget : public QWidget {
  Q_OBJECT
 public:
  CefWidget(Cef *cef, const QString &url = "", QWidget *parent = nullptr);
  ~CefWidget();

  // If result is non-null, it needs to replace this widget
  QPointer<QWidget> OverrideWidget();\
  void LoadUrl(const QString &url);
  void UpdateSize();

 protected:
  void moveEvent(QMoveEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  Cef *cef_;
  CefRefPtr<CefHandler> handler_;
  CefRefPtr<CefBrowser> browser_;
  QPointer<QWidget> override_widget_;
  bool download_favicon_;

  void InitBrowser(const QString &url);

  class FaviconDownloadCallback : public CefDownloadImageCallback {
   public:
    explicit FaviconDownloadCallback(QPointer<CefWidget> cef_widg)
      : cef_widg_(cef_widg) {}
    void OnDownloadImageFinished(const CefString& image_url,
                                 int http_status_code,
                                 CefRefPtr<CefImage> image) override;
   private:
    QPointer<CefWidget> cef_widg_;
    IMPLEMENT_REFCOUNTING(FaviconDownloadCallback);
    DISALLOW_COPY_AND_ASSIGN(FaviconDownloadCallback);
  };

 signals:
  void UrlChanged(const QString &url);
  void TitleChanged(const QString &title);
  void StatusChanged(const QString &status);
  void FaviconChanged(const QIcon &icon);
  void TabOpen(CefRequestHandler::WindowOpenDisposition type,
               const QString &url,
               bool user_gesture);
};

#endif // DOOGIE_CEFWIDGET_H_
