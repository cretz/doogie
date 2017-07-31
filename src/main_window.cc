#include "main_window.h"
#include "browser_stack.h"
#include "util.h"
#include "action_manager.h"
#include "profile.h"
#include "profile_settings_dialog.h"
#include "profile_change_dialog.h"

namespace doogie {

MainWindow* MainWindow::instance_ = nullptr;

MainWindow::MainWindow(Cef* cef, QWidget* parent)
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

  browser_stack_ = new BrowserStack(cef, this);
  connect(browser_stack_, &BrowserStack::ShowDevToolsRequest,
          [this](BrowserWidget* browser, const QPoint& inspect_at) {
    ShowDevTools(browser, inspect_at, true);
  });
  setCentralWidget(browser_stack_);
  // If we're trying to close and one was cancelled, we're no
  //  longer trying to close
  connect(browser_stack_, &BrowserStack::BrowserCloseCancelled,
          [this](BrowserWidget*) {
    attempting_to_close_ = false;
    launch_with_profile_on_close = "";
  });

  page_tree_dock_ = new PageTreeDock(browser_stack_, this);
  page_tree_dock_->setObjectName("page_tree_dock");
  addDockWidget(Qt::LeftDockWidgetArea, page_tree_dock_);
  // If we're attempting to close and the stack becomes empty,
  //  try the close again
  connect(page_tree_dock_, &PageTreeDock::TreeEmpty, [this]() {
    if (attempting_to_close_) {
      attempting_to_close_ = false;
      this->close();
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
  qInstallMessageHandler(MainWindow::LogQtMessage);

  resizeDocks({dev_tools_dock_, logging_dock_}, {300, 300}, Qt::Vertical);

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

QJsonObject MainWindow::DebugDump() {
  QJsonObject menus;
  auto common_height = 0;
  for (const auto& menu : menuBar()->findChildren<QMenu*>()) {
    if (!menu->title().isEmpty()) {
      auto menu_rect = menuBar()->actionGeometry(menu->menuAction());
      QJsonObject items;
      for (const auto& action : menu->actions()) {
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
  };
  return {
    { "windowTitle", this->windowTitle() },
    { "rect", Util::DebugWidgetGeom(this) },
    { "pageTree", page_tree_dock_->DebugDump() },
    { "menu", QJsonObject({
      { "itemHeight", common_height },
      { "items", menus }
    })}
  };
}

void MainWindow::closeEvent(QCloseEvent* event) {
  // We only let close go through if there are no open pages
  if (page_tree_dock_->HasOpenPages()) {
    attempting_to_close_ = true;
    ActionManager::Action(ActionManager::CloseAllPages)->trigger();
    event->ignore();
    return;
  }
  // Save the state of the windows
  QSettings settings;
  settings.setValue("mainWin/geom", saveGeometry());
  settings.setValue("mainWin/state", saveState(kStateVersion));
  if (!launch_with_profile_on_close.isEmpty()) {
    if (!Profile::LaunchWithProfile(launch_with_profile_on_close)) {
      launch_with_profile_on_close = "";
      event->ignore();
      return;
    }
  }
  QMainWindow::closeEvent(event);
}

void MainWindow::dropEvent(QDropEvent* event) {
  for (const auto& url : event->mimeData()->urls()) {
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
  cef_->Tick();
}

void MainWindow::SetupActions() {
  // Actions
  auto profile_settings =
      ActionManager::Action(ActionManager::ProfileSettings);
  connect(profile_settings, &QAction::triggered, [this]() {
    ProfileSettingsDialog dialog(Profile::Current(), this);
    if (dialog.exec() == QDialog::Accepted && dialog.NeedsRestart()) {
      launch_with_profile_on_close = Profile::Current()->Path();
      this->close();
    }
  });
  profile_settings->setText(QString("Profile Settings for '") +
              Profile::Current()->FriendlyName() + "'");
  profile_settings->setEnabled(!Profile::Current()->InMemory());
  connect(ActionManager::Action(ActionManager::ChangeProfile),
          &QAction::triggered, [this]() {
    ProfileChangeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
      if (dialog.NeedsRestart()) {
        launch_with_profile_on_close = dialog.ChosenPath();
        this->close();
      } else {
        Profile::LaunchWithProfile(dialog.ChosenPath());
      }
    }
  });
  connect(ActionManager::Action(ActionManager::ToggleDevTools),
          &QAction::triggered, [this]() {
    ShowDevTools(browser_stack_->CurrentBrowser(), QPoint(), false);
  });
  auto logs_action = ActionManager::Action(ActionManager::LogsWindow);
  connect(logs_action, &QAction::triggered, [this]() {
    logging_dock_->setVisible(!logging_dock_->isVisible());
  });
  logs_action->setCheckable(true);
  connect(logging_dock_, &QDockWidget::visibilityChanged,
          [this, logs_action](bool visible) {
    logs_action->setChecked(visible);
  });

  // Shortcut keys
  Profile::Current()->ApplyActionShortcuts();

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

  // Non-visible actions
  addAction(ActionManager::Action(ActionManager::NewChildBackgroundPage));
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

void MainWindow::LogQtMessage(QtMsgType type,
                              const QMessageLogContext& ctx,
                              const QString& str) {
  // We accept the lack of thread safety here
  auto inst = MainWindow::instance_;
  if (inst) inst->logging_dock_->LogQtMessage(type, ctx, str);
}

}  // namespace doogie
