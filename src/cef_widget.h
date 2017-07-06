#ifndef DOOGIE_CEFWIDGET_H_
#define DOOGIE_CEFWIDGET_H_

#include <QtWidgets>
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "cef.h"
#include "cef_handler.h"

class CefWidget : public QWidget {
  Q_OBJECT
 public:
  CefWidget(Cef *cef, QWidget *parent = nullptr);
  ~CefWidget();

  // If result is non-null, it needs to replace this widget
  QPointer<QWidget> OverrideWidget();

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

  void InitBrowser();

 signals:
  void UrlChanged(const QString &url);
};

#endif // DOOGIE_CEFWIDGET_H_
