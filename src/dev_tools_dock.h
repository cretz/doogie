#ifndef DOOGIE_DEV_TOOLS_DOCK_H_
#define DOOGIE_DEV_TOOLS_DOCK_H_

#include <QtWidgets>

#include "cef/cef_base_widget.h"
#include "browser_stack.h"
#include "browser_widget.h"

namespace doogie {

// Dock window for dev tools.
class DevToolsDock : public QDockWidget {
  Q_OBJECT

 public:
  explicit DevToolsDock(const Cef& cef,
                        BrowserStack* browser_stack,
                        QWidget* parent = nullptr);

  bool DevToolsShowing() const;

  void BrowserChanged(BrowserWidget* browser);
  void ShowDevTools(BrowserWidget* browser, const QPoint& inspect_at);
  void CloseDevTools(BrowserWidget* browser);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  void DevToolsClosed(BrowserWidget* browser);

  const Cef& cef_;
  BrowserStack* browser_stack_;
  QStackedWidget* tools_stack_;
  QHash<BrowserWidget*, CefBaseWidget*> tools_widgets_;
};

}  // namespace doogie

#endif  // DOOGIE_DEV_TOOLS_DOCK_H_
