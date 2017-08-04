#include "workspace_tree_item.h"
#include "page_tree.h"
#include "util.h"

namespace doogie {

WorkspaceTreeItem::WorkspaceTreeItem()
    : QTreeWidgetItem(PageTree::kWorkspaceItemType) {
  setFlags(Qt::ItemIsEditable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
  setText(0, "(New Workspace)");
}

void WorkspaceTreeItem::AfterAdded() {

  auto menu_button = new QToolButton;
  menu_button->setIcon(
        Util::CachedIcon(":/res/images/fontawesome/bars.png"));
  menu_button->setAutoRaise(true);
  treeWidget()->connect(menu_button, &QToolButton::clicked, [=]() {
    QMenu menu;
    ApplyMenu(&menu);
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

void WorkspaceTreeItem::ApplyMenu(QMenu* menu) {
  menu->addAction("Change Workspace Name", [=]() {
    treeWidget()->editItem(this);
  });
  menu->addAction("Close Workspace");
  menu->addAction("Delete Workspace");
}

}  // namespace doogie
