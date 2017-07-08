#include "page_tree_dock.h"

PageTreeDock::PageTreeDock(BrowserStack *browser_stack, QWidget *parent)
    : QDockWidget("Pages", parent) {
  setFeatures(QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable);

  // We're going to make a main window inside of here for toolbar reasons
  auto inner_win = new QMainWindow();
  auto toolbar = new QToolBar(inner_win);
  auto spacer = new QWidget;
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  toolbar->addWidget(spacer);
  inner_win->addToolBar(toolbar);

  // Create tree
  tree_ = new PageTree(browser_stack, inner_win);
  inner_win->setCentralWidget(tree_);
  toolbar->addAction(QIcon(":/res/images/fontawesome/plus.png"), "New Page", [this]() {
    tree_->NewBrowser();
  });

  setWidget(inner_win);
}
