#include "main_window.h"
#include "page_tree_dock.h"
#include "browser_stack.h"
#include "dev_tools_dock.h"

namespace doogie {

MainWindow::MainWindow(Cef* cef, QWidget* parent)
    : QMainWindow(parent), cef_(cef) {
  // TODO: how to determine best interval
  // TODO: is the timer stopped for us?
  if (startTimer(10) == 0) {
    throw std::runtime_error("Unable to start CEF timer");
  }

  // Common settings
  setWindowTitle("Doogie");
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  resize(1024, 768);

  auto browser_stack = new BrowserStack(cef, this);
  setCentralWidget(browser_stack);

  auto page_tree = new PageTreeDock(browser_stack, this);
  addDockWidget(Qt::LeftDockWidgetArea, page_tree);

  auto dev_tools = new DevToolsDock(cef, browser_stack, this);
  dev_tools->setVisible(false);
  addDockWidget(Qt::BottomDockWidgetArea, dev_tools);

  resizeDocks({dev_tools}, {300}, Qt::Vertical);

  // We choose for verticle windows to occupy the corners
  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Setup the menu options...
  // TODO: configurable hotkeys and better organization
  auto pages_menu = menuBar()->addMenu("&Pages");
  pages_menu->addAction("New Top-Level Page", [this, page_tree]() {
    page_tree->NewTopLevelPage("");
  })->setShortcut(Qt::CTRL + Qt::Key_T);
  pages_menu->addAction("New Child Page", [this, page_tree]() {
    page_tree->NewChildPage("");
  })->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_T);
  pages_menu->addAction("Close Page", [this, page_tree]() {
    page_tree->CloseCurrentPage();
  })->setShortcuts({Qt::CTRL + Qt::Key_F4, Qt::CTRL + Qt::Key_W});
  pages_menu->addAction("Close All Pages", [this, page_tree]() {
    page_tree->CloseAllPages();
  })->setShortcuts({Qt::CTRL + Qt::SHIFT + Qt::Key_F4,
                    Qt::CTRL + Qt::SHIFT + Qt::Key_W});
  pages_menu->addAction("Toggle Dev Tools",
                        [this, browser_stack, dev_tools]() {
    // Here's how it works:
    // * F12 (Ctrl or not) - If tools not open, open dock if not open then tools
    // * F12 - If tools open, close tools and dock
    // * Ctrl F12 - Close just the tools
    auto curr_browser = browser_stack->CurrentBrowser();
    auto tools_showing = dev_tools->isVisible() &&
        (dev_tools->DevToolsShowing() || !curr_browser);
    if (!tools_showing) {
      if (!dev_tools->isVisible()) dev_tools->show();
      if (curr_browser) dev_tools->ShowDevTools(curr_browser);
    } else {
      if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        if (curr_browser) dev_tools->CloseDevTools(curr_browser);
      } else {
        dev_tools->close();
      }
    }
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
  win_menu->addAction("Focus Page Tree", [this, page_tree]() {
    page_tree->FocusPageTree();
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

void MainWindow::keyPressEvent(QKeyEvent* event) {
  QMainWindow::keyPressEvent(event);
}

void MainWindow::timerEvent(QTimerEvent*) {
  cef_->Tick();
}

}  // namespace doogie
