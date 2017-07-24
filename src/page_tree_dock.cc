#include "page_tree_dock.h"
#include "util.h"

namespace doogie {

PageTreeDock::PageTreeDock(BrowserStack* browser_stack, QWidget* parent)
    : QDockWidget("Pages", parent) {
  setFeatures(QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable);

  // Create tree
  tree_ = new PageTree(browser_stack, this);
  connect(tree_, &PageTree::TreeEmpty, this, &PageTreeDock::TreeEmpty);
  setFocusProxy(tree_);
  setWidget(tree_);
}

void PageTreeDock::NewPage(const QString &url,
                           bool top_level,
                           bool make_current) {
  tree_->NewPage(url,
                 (top_level) ? nullptr : tree_->CurrentItem(),
                 make_current);
}

bool PageTreeDock::HasOpenPages() {
  return tree_->topLevelItemCount() > 0;
}

QJsonObject PageTreeDock::DebugDump() {
  return tree_->DebugDump();
}

}  // namespace doogie
