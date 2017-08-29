#include "workspace_tree_item.h"
#include "page_tree.h"
#include "util.h"

namespace doogie {

WorkspaceTreeItem::WorkspaceTreeItem(const Workspace& workspace)
    : QTreeWidgetItem(PageTree::kWorkspaceItemType), workspace_(workspace) {
  setFlags(Qt::ItemIsEditable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
  setText(0, workspace.FriendlyName());
}

WorkspaceTreeItem::~WorkspaceTreeItem() {
  if (delete_workspace_on_destroy_not_cancelled_) {
    workspace_.Delete();
  }
}

void WorkspaceTreeItem::AfterAdded() {
  auto menu_button = new QToolButton;
  menu_button->setIcon(
        Util::CachedIcon(":/res/images/fontawesome/bars.png"));
  menu_button->setAutoRaise(true);
  treeWidget()->connect(menu_button, &QToolButton::clicked, [=]() {
    auto tree = static_cast<PageTree*>(treeWidget());
    QMenu menu;
    tree->ApplyWorkspaceMenu(&menu, workspace_, this);
    menu.exec(menu_button->mapToGlobal(menu_button->rect().bottomLeft()));
  });
  treeWidget()->setItemWidget(this,
                              PageTreeItem::kCloseButtonColumn,
                              menu_button);

  // Gotta call it on my children too sadly
  for (int i = 0; i < childCount(); i++) {
    static_cast<PageTreeItem*>(child(i))->AfterAdded();
  }
}

void WorkspaceTreeItem::TextChanged() {
  if (text(0) != workspace_.Name()) {
    if (Workspace::NameInUse(text(0))) {
      QMessageBox::critical(nullptr,
                            "Invalid Name",
                            "Name already in use by another workspace");
      setText(0, workspace_.Name());
      return;
    }
    workspace_.SetName(text(0));
    workspace_.Persist();
  }
}

void WorkspaceTreeItem::ChildCloseCancelled() {
  close_on_empty_not_cancelled_ = false;
  send_close_event_not_cancelled_ = true;
  delete_workspace_on_destroy_not_cancelled_ = false;
}

void WorkspaceTreeItem::ChildCloseCompleted() {
  if (close_on_empty_not_cancelled_ && childCount() == 0) {
    auto tree = static_cast<PageTree*>(treeWidget());
    auto id = workspace_.Id();
    auto send_not_cancelled = send_close_event_not_cancelled_;
    tree->WorkspaceAboutToDestroy(this);
    delete this;
    if (send_not_cancelled) emit tree->WorkspaceClosed(id);
  }
}

}  // namespace doogie
