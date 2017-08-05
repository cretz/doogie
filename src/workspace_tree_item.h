#ifndef DOOGIE_WORKSPACE_TREE_ITEM_H_
#define DOOGIE_WORKSPACE_TREE_ITEM_H_

#include <QtWidgets>

namespace doogie {

class WorkspaceTreeItem : public QTreeWidgetItem {
 public:
  WorkspaceTreeItem();

  void AfterAdded();
  void ApplyMenu(QMenu* menu);
};

}  // namespace doogie

#endif  // DOOGIE_WORKSPACE_TREE_ITEM_H_
