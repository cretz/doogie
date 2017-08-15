#ifndef DOOGIE_MAIN_WINDOW_H_
#define DOOGIE_MAIN_WINDOW_H_

#include <QtWidgets>

#include "browser_stack.h"
#include "cef/cef.h"
#include "cef/cef_widget.h"
#include "dev_tools_dock.h"
#include "downloads_dock.h"
#include "logging_dock.h"
#include "page_tree_dock.h"

namespace doogie {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(const Cef& cef, QWidget* parent = nullptr);
  ~MainWindow();

  QJsonObject DebugDump() const;

 protected:
  void closeEvent(QCloseEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void timerEvent(QTimerEvent* event) override;

 private:
  static void LogQtMessage(QtMsgType type,
                           const QMessageLogContext& ctx,
                           const QString& str);
  void SetupActions();
  void ShowDevTools(BrowserWidget* browser,
                    const QPoint& inspect_at,
                    bool force_open);

  const Cef& cef_;
  BrowserStack* browser_stack_;
  PageTreeDock* page_tree_dock_;
  DownloadsDock* downloads_dock_;
  DevToolsDock* dev_tools_dock_;
  LoggingDock* logging_dock_;
  bool attempting_to_close_ = false;
  bool ok_with_terminating_downloads_ = false;
  QString launch_with_profile_on_close;
  static MainWindow* instance_;
  static const int kStateVersion = 1;
};

}  // namespace doogie

#endif  // DOOGIE_MAIN_WINDOW_H_
