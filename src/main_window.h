#ifndef DOOGIE_MAIN_WINDOW_H_
#define DOOGIE_MAIN_WINDOW_H_

#include <QtWidgets>

#include "blocker_dock.h"
#include "browser_stack.h"
#include "cef/cef.h"
#include "cef/cef_widget.h"
#include "dev_tools_dock.h"
#include "downloads_dock.h"
#include "logging_dock.h"
#include "page_tree_dock.h"
#include "profile_settings_dialog.h"

namespace doogie {

// Primary window. This is currently a singleton.
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(const Cef& cef, QWidget* parent = nullptr);
  ~MainWindow();

  static void EditProfileSettings(std::function<void(ProfileSettingsDialog*)>
                                  adjust_dialog_before_exec = nullptr);

  QJsonObject DebugDump() const;

 protected:
  void closeEvent(QCloseEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void timerEvent(QTimerEvent* event) override;

 private:
  static const int kStateVersion = 1;

  static void LogQtMessage(QtMsgType type,
                           const QMessageLogContext& ctx,
                           const QString& str);
  void SetupActions();
  void ShowDevTools(BrowserWidget* browser,
                    const QPoint& inspect_at,
                    bool force_open);

  static MainWindow* instance_;

  const Cef& cef_;
  BrowserStack* browser_stack_;
  PageTreeDock* page_tree_dock_;
  DownloadsDock* downloads_dock_;
  DevToolsDock* dev_tools_dock_;
  BlockerDock* blocker_dock_;
  LoggingDock* logging_dock_;
  bool attempting_to_close_ = false;
  bool ok_with_terminating_downloads_ = false;
  QString launch_with_profile_on_close_;
  QTimer crl_check_timer_;
};

}  // namespace doogie

#endif  // DOOGIE_MAIN_WINDOW_H_
