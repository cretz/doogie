#ifndef DOOGIE_CEFWIDGET_H_
#define DOOGIE_CEFWIDGET_H_

#include <QtWidgets>
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "cef.h"

class CefWidget : public QWidget {
  Q_OBJECT
 public:
  CefWidget(Cef *cef, QWidget *parent = 0);
  ~CefWidget();

  // If result is non-null, it needs to replace this widget
  QPointer<QWidget> EmbedBrowser(QMainWindow *main_win,
                                 QLineEdit *url_line_edit);
  void LoadUrl(const QString &url);

  void UpdateSize();

 protected:
  void moveEvent(QMoveEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:

  Cef *cef_;
  CefRefPtr<CefBrowser> browser_;
};

#endif // DOOGIE_CEFWIDGET_H_
