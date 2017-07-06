#ifndef DOOGIE_BROWSERWIDGET_H_
#define DOOGIE_BROWSERWIDGET_H_

#include <QtWidgets>
#include "cef_widget.h"

class BrowserWidget : public QWidget {
  Q_OBJECT
 public:
  explicit BrowserWidget(Cef *cef, QWidget *parent = nullptr);

 private:
  Cef *cef_;
  QLineEdit *url_edit_;
  CefWidget *cef_widg_;
};

#endif // DOOGIE_BROWSERWIDGET_H_
