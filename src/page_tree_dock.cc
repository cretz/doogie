#include "page_tree_dock.h"
#include "util.h"

PageTreeDock::PageTreeDock(BrowserStack *browser_stack, QWidget *parent)
    : QDockWidget("Pages", parent) {
  setFeatures(QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable);

  // We're going to make a main window inside of here for toolbar reasons
  auto inner_win = new QMainWindow();
  auto toolbar = new QToolBar(inner_win);
  toolbar->setMovable(false);
  toolbar->setIconSize(QSize(16, 16));
  auto spacer = new QWidget;
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  inner_win->addToolBar(toolbar);

  // Create tree
  tree_ = new PageTree(browser_stack, inner_win);
  inner_win->setCentralWidget(tree_);

  toolbar->addAction(
        Util::CachedIcon(":/res/images/fontawesome/plus.png"),
        "New Page",
        [this]() {
    if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
      NewChildPage("");
    } else {
      NewTopLevelPage("");
    }
  });
  toolbar->addWidget(spacer);

  setWidget(inner_win);
}

void PageTreeDock::FocusPageTree() {
  tree_->setFocus();
}

void PageTreeDock::NewTopLevelPage(const QString &url) {
  tree_->NewPage(url, true);
}

void PageTreeDock::NewChildPage(const QString &url) {
  tree_->NewPage(url, false);
}

void PageTreeDock::CloseCurrentPage() {
  tree_->CloseCurrentPage();
}

void PageTreeDock::CloseAllPages() {
  tree_->CloseAllPages();
}
