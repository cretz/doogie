#ifndef DOOGIE_PAGE_TREE_ITEM_H_
#define DOOGIE_PAGE_TREE_ITEM_H_

#include <QtWidgets>

#include "browser_widget.h"
#include "workspace.h"
#include "workspace_tree_item.h"

namespace doogie {

// A single item in the page tree.
class PageTreeItem : public QTreeWidgetItem {
 public:
  static const int kBubbleIconColumn = 1;
  static const int kCloseButtonColumn = 2;

  explicit PageTreeItem(QPointer<BrowserWidget> browser,
                        const Workspace::WorkspacePage& workspace_page);
  ~PageTreeItem();
  QPointer<BrowserWidget> Browser() const;
  QToolButton* CloseButton() const;
  void AfterAdded();

  PageTreeItem* Parent() const;
  const Workspace::WorkspacePage& WorkspacePage() const {
    return workspace_page_;
  }

  QJsonObject DebugDump() const;

  bool SelfOrAnyChildCollapsed() const;
  bool SelfOrAnyChildExpanded() const;
  void ExpandSelfAndChildren();
  void CollapseSelfAndChildren();
  QList<PageTreeItem*> SelfSelectedOrChildrenSelected() const;
  bool SelectedOrHasSelectedParent() const;
  void ApplyWorkspaceExpansion();

  QList<PageTreeItem*> Siblings() const;

  void SetCurrentBubbleIfDifferent(const Bubble& bubble);

  void SetPersistNextCloseToWorkspace(bool persist) {
    persist_next_close_to_workspace_ = persist;
  }

  const Workspace& CurrentWorkspace();
  WorkspaceTreeItem* WorkspaceItem();

  void CollapseStateChanged();

 private:
  void ApplyFavicon(const QIcon& icon_override = QIcon());

  QPointer<BrowserWidget> browser_;
  Workspace::WorkspacePage workspace_page_;
  QToolButton* close_button_ = nullptr;
  QMetaObject::Connection loading_icon_frame_conn_;
  bool persist_next_close_to_workspace_ = true;
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_TREE_ITEM_H_
