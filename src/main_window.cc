#include "main_window.h"

#include "action_manager.h"
#include "browser_stack.h"
#include "profile.h"
#include "profile_change_dialog.h"
#include "profile_settings_dialog.h"
#include "util.h"

namespace doogie {

MainWindow* MainWindow::instance_ = nullptr;

MainWindow::MainWindow(const Cef& cef, QWidget* parent)
    : QMainWindow(parent), cef_(cef) {
  if (instance_ == nullptr) instance_ = this;

  // TODO(cretz): how to determine best interval
  // TODO(cretz): is the timer stopped for us?
  if (startTimer(10) == 0) {
    throw std::runtime_error("Unable to start CEF timer");
  }

  // Common settings
  setWindowTitle("Doogie");
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  resize(1024, 768);
  setAcceptDrops(true);
  setWindowIcon(QIcon(":/res/images/doogie-icon.ico"));

  browser_stack_ = new BrowserStack(cef, this);
  connect(browser_stack_, &BrowserStack::ShowDevToolsRequest,
          [=](BrowserWidget* browser, const QPoint& inspect_at) {
    ShowDevTools(browser, inspect_at, true);
  });
  setCentralWidget(browser_stack_);
  // If we're trying to close and one was cancelled, we're no
  //  longer trying to close
  connect(browser_stack_, &BrowserStack::BrowserCloseCancelled,
          [=](BrowserWidget*) {
    attempting_to_close_ = false;
    ok_with_terminating_downloads_ = false;
    launch_with_profile_on_close_ = "";
  });

  downloads_dock_ = new DownloadsDock(browser_stack_, this);
  downloads_dock_->setObjectName("downloads_dock");
  downloads_dock_->setVisible(false);
  addDockWidget(Qt::LeftDockWidgetArea, downloads_dock_);

  blocker_dock_ = new BlockerDock(cef, browser_stack_, this);
  blocker_dock_->setObjectName("blocker_dock");
  blocker_dock_->setVisible(false);
  addDockWidget(Qt::BottomDockWidgetArea, blocker_dock_);

  page_tree_dock_ = new PageTreeDock(browser_stack_, this);
  page_tree_dock_->setObjectName("page_tree_dock");
  addDockWidget(Qt::LeftDockWidgetArea, page_tree_dock_);
  // If we're attempting to close and the stack becomes empty,
  //  try the close again
  connect(page_tree_dock_, &PageTreeDock::TreeEmpty, [=]() {
    if (attempting_to_close_) {
      attempting_to_close_ = false;
      // We have to defer this because closeEvent will not be called
      // again inside of itself
      QTimer::singleShot(0, [=]() { close(); });
    }
  });

  dev_tools_dock_ = new DevToolsDock(cef, browser_stack_, this);
  dev_tools_dock_->setObjectName("dev_tools_dock");
  dev_tools_dock_->setVisible(false);
  addDockWidget(Qt::BottomDockWidgetArea, dev_tools_dock_);

  logging_dock_ = new LoggingDock(this);
  logging_dock_->setObjectName("logging_dock");
  logging_dock_->setVisible(false);
  addDockWidget(Qt::BottomDockWidgetArea, logging_dock_);
  // We'll let the console log on debug
#ifndef QT_DEBUG
  qInstallMessageHandler(MainWindow::LogQtMessage);
#endif

  resizeDocks({ blocker_dock_, dev_tools_dock_, logging_dock_ },
              { 300, 300, 300 },
              Qt::Vertical);
  resizeDocks({ downloads_dock_, page_tree_dock_ },
              { 300, 300 },
              Qt::Horizontal);

  // We choose for verticle windows to occupy the corners
  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  SetupActions();

  // Restore the window state
  QSettings settings;
  restoreGeometry(settings.value("mainWin/geom").toByteArray());
  restoreState(settings.value("mainWin/state").toByteArray(),
               kStateVersion);
}

MainWindow::~MainWindow() {
  instance_ = nullptr;
}

void MainWindow::EditProfileSettings(
    std::function<void(ProfileSettingsDialog*)> adjust_dialog_before_exec) {
  QSet<qlonglong> in_use_ids;
  for (auto browser : instance_->browser_stack_->Browsers()) {
    in_use_ids << browser->CurrentBubble().Id();
  }
  ProfileSettingsDialog dialog(instance_->cef_, in_use_ids, instance_);
  if (adjust_dialog_before_exec) {
    // We still show this first so it will appear in the background
    dialog.show();
    adjust_dialog_before_exec(&dialog);
  }
  if (dialog.exec() == QDialog::Accepted) {
    if (QDialog::Accepted && dialog.NeedsRestart()) {
      instance_->launch_with_profile_on_close_ = Profile::Current().Path();
      instance_->close();
    } else {
      instance_->blocker_dock_->ProfileUpdated();
    }
  }
}

QJsonObject MainWindow::DebugDump() const {
  QJsonObject menus;
  auto common_height = 0;
  for (auto menu : menuBar()->findChildren<QMenu*>()) {
    if (!menu->title().isEmpty()) {
      auto menu_rect = menuBar()->actionGeometry(menu->menuAction());
      QJsonObject items;
      for (auto action : menu->actions()) {
        if (!action->text().isEmpty()) {
          auto action_rect = menu->actionGeometry(action);
          action_rect.moveTop(action_rect.top() + menu_rect.height());
          common_height = action_rect.height();
          items[action->text()] = QJsonObject({
            { "enabled", action->isEnabled() },
            { "rect", Util::DebugWidgetGeom(this, action_rect) }
          });
        }
      }
      menus[menu->title()] = QJsonObject({
        { "visible", menu->isVisible() },
        { "rect", Util::DebugWidgetGeom(
          menuBar(), menu_rect) },
        { "items", items }
      });
    }
  }
  return {
    { "windowTitle", windowTitle() },
    { "rect", Util::DebugWidgetGeom(this) },
    { "pageTree", page_tree_dock_->DebugDump() },
    { "menu", QJsonObject({
      { "itemHeight", common_height },
      { "items", menus }
    })}
  };
}

void MainWindow::closeEvent(QCloseEvent* event) {
  // Warn the user about active/paused downloads
  if (downloads_dock_->HasActiveDownload() &&
      !ok_with_terminating_downloads_) {
    auto but = QMessageBox::question(
          nullptr, "Doogie",
          "Some downloads are still active/paused.\n"
          "Closing Doogie will terminate the download "
          "part-way causing file corruption.\n"
          "Are you sure you want to continue closing?");
    if (but != QMessageBox::Yes) {
      event->ignore();
      return;
    }
    ok_with_terminating_downloads_ = true;
  }
  // We only let close go through if there are no open pages
  if (page_tree_dock_->HasTopLevelItems()) {
    attempting_to_close_ = true;
    page_tree_dock_->CloseAllWorkspaces();
    event->ignore();
    return;
  }
  // Save the state of the windows
  QSettings settings;
  settings.setValue("mainWin/geom", saveGeometry());
  settings.setValue("mainWin/state", saveState(kStateVersion));
  if (!launch_with_profile_on_close_.isEmpty()) {
    if (!Profile::LaunchWithProfile(launch_with_profile_on_close_)) {
      launch_with_profile_on_close_ = "";
      event->ignore();
      return;
    }
  }
  QMainWindow::closeEvent(event);
}

void MainWindow::dropEvent(QDropEvent* event) {
  for (auto url : event->mimeData()->urls()) {
    page_tree_dock_->NewPage(url.url(), true, false);
  }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls() &&
      event->proposedAction() == Qt::CopyAction) {
    event->acceptProposedAction();
  }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  QMainWindow::keyPressEvent(event);
}

void MainWindow::timerEvent(QTimerEvent*) {
  cef_.Tick();
}

void MainWindow::LogQtMessage(QtMsgType type,
                              const QMessageLogContext& ctx,
                              const QString& str) {
  // We accept the lack of thread safety here
  auto inst = MainWindow::instance_;
  if (inst) inst->logging_dock_->LogQtMessage(type, ctx, str);
}

void MainWindow::SetupActions() {
  // Actions
  auto profile_settings =
      ActionManager::Action(ActionManager::ProfileSettings);
  connect(profile_settings, &QAction::triggered, [=]() {
    EditProfileSettings();
  });
  profile_settings->setText(QString("Profile Settings for '") +
              Profile::Current().FriendlyName() + "'");
  profile_settings->setEnabled(!Profile::Current().InMemory());
  connect(ActionManager::Action(ActionManager::ChangeProfile),
          &QAction::triggered, [=]() {
    ProfileChangeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
      if (dialog.NeedsRestart()) {
        launch_with_profile_on_close_ = dialog.ChosenPath();
        this->close();
      } else {
        Profile::LaunchWithProfile(dialog.ChosenPath());
      }
    }
  });
  connect(ActionManager::Action(ActionManager::ToggleDevTools),
          &QAction::triggered, [=]() {
    ShowDevTools(browser_stack_->CurrentBrowser(), QPoint(), false);
  });
  auto downloads_action =
      ActionManager::Action(ActionManager::DownloadsWindow);
  connect(downloads_action, &QAction::triggered, [=]() {
    downloads_dock_->setVisible(!downloads_dock_->isVisible());
  });
  downloads_action->setCheckable(true);
  connect(downloads_dock_, &QDockWidget::visibilityChanged, [=](bool visible) {
    downloads_action->setChecked(visible);
  });

  auto blocker_action =
      ActionManager::Action(ActionManager::BlockerWindow);
  connect(blocker_action, &QAction::triggered, [=]() {
    blocker_dock_->setVisible(!blocker_dock_->isVisible());
  });
  blocker_action->setCheckable(true);
  connect(blocker_dock_, &QDockWidget::visibilityChanged, [=](bool visible) {
    blocker_action->setChecked(visible);
  });

  auto logs_action = ActionManager::Action(ActionManager::LogsWindow);
  connect(logs_action, &QAction::triggered, [=]() {
    logging_dock_->setVisible(!logging_dock_->isVisible());
  });
  logs_action->setCheckable(true);
  connect(logging_dock_, &QDockWidget::visibilityChanged, [=](bool visible) {
    logs_action->setChecked(visible);
  });

  // Shortcut keys
  Profile::Current().ApplyActionShortcuts();

  // Setup menu
  auto menu_action = [](QMenu* menu, ActionManager::Type type) {
    menu->addAction(ActionManager::Action(type));
  };

  auto menu = menuBar()->addMenu("P&ages");
  menu_action(menu, ActionManager::NewTopLevelPage);
  menu_action(menu, ActionManager::NewChildForegroundPage);
  menu_action(menu, ActionManager::ClosePage);
  menu_action(menu, ActionManager::CloseAllPages);
  menu_action(menu, ActionManager::ToggleDevTools);
  menu = menuBar()->addMenu("&Page");
  menu_action(menu, ActionManager::Reload);
  menu_action(menu, ActionManager::Stop);
  menu_action(menu, ActionManager::Back);
  menu_action(menu, ActionManager::Forward);
  menu_action(menu, ActionManager::Print);
  menu_action(menu, ActionManager::Fullscreen);
  menu_action(menu, ActionManager::ZoomIn);
  menu_action(menu, ActionManager::ZoomOut);
  menu_action(menu, ActionManager::ResetZoom);
  menu_action(menu, ActionManager::FindInPage);
  menu = menuBar()->addMenu("&Window");
  auto profile_menu = menu->addMenu("Profile");
  menu_action(profile_menu, ActionManager::ProfileSettings);
  menu_action(profile_menu, ActionManager::ChangeProfile);
  menu_action(menu, ActionManager::FocusPageTree);
  menu_action(menu, ActionManager::FocusAddressBar);
  menu_action(menu, ActionManager::FocusBrowser);
  menu_action(menu, ActionManager::LogsWindow);
  menu_action(menu, ActionManager::DownloadsWindow);
  menu_action(menu, ActionManager::BlockerWindow);

  // Non-visible actions
  addAction(ActionManager::Action(ActionManager::NewChildBackgroundPage));
  addAction(ActionManager::Action(ActionManager::NextPage));
  addAction(ActionManager::Action(ActionManager::PreviousPage));
}

void MainWindow::ShowDevTools(BrowserWidget* browser,
                              const QPoint& inspect_at,
                              bool force_open) {
  // Here's how it works:
  // * F12 (Ctrl or not) - If tools not open, open dock if not open then tools
  // * F12 - If tools open, close tools and dock
  // * Ctrl F12 - Close just the tools
  auto should_close = !force_open && dev_tools_dock_->isVisible() &&
      (dev_tools_dock_->DevToolsShowing() || !browser);
  if (!should_close) {
    if (!dev_tools_dock_->isVisible()) dev_tools_dock_->show();
    if (browser) dev_tools_dock_->ShowDevTools(browser, inspect_at);
  } else {
    if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
      if (browser) dev_tools_dock_->CloseDevTools(browser);
    } else {
      dev_tools_dock_->close();
    }
  }
}

}  // namespace doogie
