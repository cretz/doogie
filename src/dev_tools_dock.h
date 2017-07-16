#ifndef DOOGIE_DEV_TOOLS_DOCK_H_
#define DOOGIE_DEV_TOOLS_DOCK_H_

#include <QtWidgets>
#include "browser_stack.h"
#include "browser_widget.h"

namespace doogie {

class DevToolsDock : public QDockWidget {
  Q_OBJECT
 public:
  explicit DevToolsDock(Cef* cef,
                        BrowserStack* browser_stack,
                        QWidget* parent = nullptr);

  bool DevToolsShowing();

 public slots:  // NOLINT(whitespace/indent)
  void BrowserChanged(BrowserWidget* browser);
  void ShowDevTools(BrowserWidget* browser);
  void CloseDevTools(BrowserWidget* browser);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  void DevToolsClosed(BrowserWidget* browser);

  Cef* cef_;
  BrowserStack* browser_stack_;
  QStackedWidget* tools_stack_;
  QHash<BrowserWidget*, QWidget*> tools_widgets_;
};

}  // namespace doogie

#endif  // DOOGIE_DEV_TOOLS_DOCK_H_
