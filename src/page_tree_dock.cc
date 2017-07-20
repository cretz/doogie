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
  setWidget(tree_);
}

void PageTreeDock::FocusPageTree() {
  tree_->setFocus();
}

void PageTreeDock::NewTopLevelPage(const QString& url) {
  tree_->NewPage(url, true);
}

void PageTreeDock::NewChildPage(const QString& url) {
  tree_->NewPage(url, false);
}

void PageTreeDock::CloseCurrentPage() {
  tree_->CloseCurrentPage();
}

void PageTreeDock::CloseAllPages() {
  tree_->CloseAllPages();
}

bool PageTreeDock::HasOpenPages() {
  return tree_->topLevelItemCount() > 0;
}

QJsonObject PageTreeDock::DebugDump() {
  return tree_->DebugDump();
}

}  // namespace doogie
