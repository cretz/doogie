#ifndef DOOGIE_BROWSERSTACK_H_
#define DOOGIE_BROWSERSTACK_H_

#include <QtWidgets>
#include "browser_widget.h"

class BrowserStack : public QStackedWidget {
  Q_OBJECT
 public:
  explicit BrowserStack(Cef *cef, QWidget *parent = nullptr);

  QPointer<BrowserWidget> NewBrowser();

 private:
  Cef *cef_;
};

#endif // DOOGIE_BROWSERSTACK_H_
