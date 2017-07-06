#include "main_window.h"
#include "page_tree.h"
#include "browser_stack.h"

MainWindow::MainWindow(Cef *cef, QWidget *parent)
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

  auto page_tree = new PageTree(browser_stack, this);
  addDockWidget(Qt::LeftDockWidgetArea, page_tree);
}

MainWindow::~MainWindow() {
}

void MainWindow::timerEvent(QTimerEvent*) {
  cef_->Tick();
}
