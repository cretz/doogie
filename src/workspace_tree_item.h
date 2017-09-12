#ifndef DOOGIE_WORKSPACE_TREE_ITEM_H_
#define DOOGIE_WORKSPACE_TREE_ITEM_H_

#include <QtWidgets>

#include "workspace.h"

namespace doogie {

// Tree item for a workspace.
class WorkspaceTreeItem : public QTreeWidgetItem {
 public:
  explicit WorkspaceTreeItem(const Workspace& workspace);
  ~WorkspaceTreeItem();

  const Workspace& CurrentWorkspace() const { return workspace_; }

  void AfterAdded();

  void TextChanged();

  void ChildCloseCompleted();

  void ChildCloseCancelled();

  void SetCloseOnEmptyNotCancelled(bool close_on_empty_not_cancelled) {
    close_on_empty_not_cancelled_ = close_on_empty_not_cancelled;
  }

  void SetSendCloseEventNotCancelled(bool send_close_event_not_cancelled) {
    send_close_event_not_cancelled_ = send_close_event_not_cancelled;
  }

  void SetDeleteWorkspaceOnDestroyNotCancelled(
      bool delete_workspace_on_destroy_not_cancelled) {
    delete_workspace_on_destroy_not_cancelled_ =
        delete_workspace_on_destroy_not_cancelled;
  }

 private:
  Workspace workspace_;
  bool close_on_empty_not_cancelled_ = false;
  bool send_close_event_not_cancelled_ = true;
  bool delete_workspace_on_destroy_not_cancelled_ = false;
};

}  // namespace doogie

#endif  // DOOGIE_WORKSPACE_TREE_ITEM_H_
