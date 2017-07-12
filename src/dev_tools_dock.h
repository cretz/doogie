#ifndef DOOGIE_DEVTOOLSDOCK_H_
#define DOOGIE_DEVTOOLSDOCK_H_

#include <QtWidgets>
#include "browser_stack.h"
#include "browser_widget.h"

class DevToolsDock : public QDockWidget {
  Q_OBJECT
 public:
  explicit DevToolsDock(BrowserStack *browser_stack,
                        QWidget *parent = nullptr);

 public slots:
  void BrowserChanged(BrowserWidget* browser);
  void ShowDevTools(BrowserWidget* browser);

 private:
  BrowserStack* browser_stack_;
  QStackedWidget* tools_stack_;
};

#endif // DOOGIE_DEVTOOLSDOCK_H_
