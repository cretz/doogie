#ifndef DOOGIE_DEVTOOLSDOCK_H_
#define DOOGIE_DEVTOOLSDOCK_H_

#include <QtWidgets>
#include "browser_stack.h"
#include "browser_widget.h"

class DevToolsDock : public QDockWidget {
  Q_OBJECT
 public:
  explicit DevToolsDock(Cef* cef,
                        BrowserStack *browser_stack,
                        QWidget *parent = nullptr);

  bool DevToolsShowing();

 public slots:
  void BrowserChanged(BrowserWidget* browser);
  void ShowDevTools(BrowserWidget* browser);
  void CloseDevTools(BrowserWidget* browser);

 protected:
  void closeEvent(QCloseEvent *event);

 private:
  Cef* cef_;
  BrowserStack* browser_stack_;
  QStackedWidget* tools_stack_;
  QHash<BrowserWidget*, QWidget*> tools_widgets_;

  void DevToolsClosed(BrowserWidget* browser);
};

#endif // DOOGIE_DEVTOOLSDOCK_H_
