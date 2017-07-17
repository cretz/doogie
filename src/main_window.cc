#include "main_window.h"
#include "browser_stack.h"
#include "util.h"

namespace doogie {

MainWindow::MainWindow(Cef* cef, QWidget* parent)
    : QMainWindow(parent), cef_(cef) {
  // TODO(cretz): how to determine best interval
  // TODO(cretz): is the timer stopped for us?
  if (startTimer(10) == 0) {
    throw std::runtime_error("Unable to start CEF timer");
  }

  // Common settings
  setWindowTitle("Doogie");
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  resize(1024, 768);

  auto browser_stack = new BrowserStack(cef, this);
  connect(browser_stack, &BrowserStack::ShowDevToolsRequest,
          [this](BrowserWidget* browser, const QPoint& inspect_at) {
    ShowDevTools(browser, inspect_at, true);
  });
  setCentralWidget(browser_stack);

  page_tree_dock_ = new PageTreeDock(browser_stack, this);
  addDockWidget(Qt::LeftDockWidgetArea, page_tree_dock_);

  dev_tools_dock_ = new DevToolsDock(cef, browser_stack, this);
  dev_tools_dock_->setVisible(false);
  addDockWidget(Qt::BottomDockWidgetArea, dev_tools_dock_);

  resizeDocks({dev_tools_dock_}, {300}, Qt::Vertical);

  // We choose for verticle windows to occupy the corners
  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Setup the menu options...
  // TODO(cretz): configurable hotkeys and better organization
  auto pages_menu = menuBar()->addMenu("&Pages");
  pages_menu->addAction("New Top-Level Page", [this]() {
    page_tree_dock_->NewTopLevelPage("");
  })->setShortcut(Qt::CTRL + Qt::Key_T);
  pages_menu->addAction("New Child Page", [this]() {
    page_tree_dock_->NewChildPage("");
  })->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_T);
  pages_menu->addAction("Close Page", [this]() {
    page_tree_dock_->CloseCurrentPage();
  })->setShortcuts({Qt::CTRL + Qt::Key_F4, Qt::CTRL + Qt::Key_W});
  pages_menu->addAction("Close All Pages", [this]() {
    page_tree_dock_->CloseAllPages();
  })->setShortcuts({Qt::CTRL + Qt::SHIFT + Qt::Key_F4,
                    Qt::CTRL + Qt::SHIFT + Qt::Key_W});
  pages_menu->addAction("Toggle Dev Tools", [this, browser_stack]() {
    ShowDevTools(browser_stack->CurrentBrowser(), QPoint(), false);
  })->setShortcuts({Qt::Key_F12, Qt::CTRL + Qt::Key_F12});

  auto page_menu = menuBar()->addMenu("&Page");
  page_menu->addAction("Refresh", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->Refresh();
  })->setShortcuts({Qt::Key_F5, Qt::CTRL + Qt::Key_F5, Qt::CTRL + Qt::Key_R});
  page_menu->addAction("Stop", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->Stop();
  })->setShortcut(Qt::Key_Escape);
  page_menu->addAction("Back", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->Back();
  })->setShortcuts({Qt::CTRL + Qt::Key_PageUp,
                    Qt::CTRL + Qt::SHIFT + Qt::Key_PageDown,
                    Qt::Key_Backspace});
  page_menu->addAction("Forward", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->Forward();
  })->setShortcuts({Qt::CTRL + Qt::Key_PageDown,
                    Qt::CTRL + Qt::SHIFT + Qt::Key_PageUp,
                    Qt::SHIFT + Qt::Key_Backspace});
  page_menu->addAction("Print", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->Print();
  })->setShortcut(Qt::CTRL + Qt::Key_P);
  page_menu->addAction("Zoom In", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) {
      curr_browser->SetZoomLevel(curr_browser->GetZoomLevel() + 0.1);
    }
  })->setShortcuts({Qt::CTRL + Qt::Key_Plus, Qt::CTRL + Qt::Key_Equal});
  page_menu->addAction("Zoom Out", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) {
      curr_browser->SetZoomLevel(curr_browser->GetZoomLevel() - 0.1);
    }
  })->setShortcut(Qt::CTRL + Qt::Key_Minus);
  page_menu->addAction("Reset Zoom", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->SetZoomLevel(0.0);
  })->setShortcut(Qt::CTRL + Qt::Key_0);
  page_menu->addAction("Find", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->ShowFind();
  })->setShortcut(Qt::CTRL + Qt::Key_F);

  auto win_menu = menuBar()->addMenu("&Window");
  win_menu->addAction("Focus Page Tree", [this]() {
    page_tree_dock_->FocusPageTree();
  })->setShortcut(Qt::ALT + Qt::Key_1);
  win_menu->addAction("Focus Address Bar", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->FocusUrlEdit();
  })->setShortcuts({Qt::ALT + Qt::Key_2, Qt::ALT + Qt::Key_D});
  win_menu->addAction("Focus Browser", [this, browser_stack]() {
    auto curr_browser = browser_stack->CurrentBrowser();
    if (curr_browser) curr_browser->FocusBrowser();
  })->setShortcut(Qt::ALT + Qt::Key_3);
}

MainWindow::~MainWindow() {
}

QJsonObject MainWindow::DebugDump() {
  return {
    { "windowTitle", this->windowTitle() },
    { "rect", Util::DebugWidgetGeom(this) },
    { "pageTree", page_tree_dock_->DebugDump() }
  };
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  QMainWindow::keyPressEvent(event);
}

void MainWindow::timerEvent(QTimerEvent*) {
  cef_->Tick();
}

void MainWindow::ShowDevTools(BrowserWidget *browser,
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
