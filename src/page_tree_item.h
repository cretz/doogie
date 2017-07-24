#ifndef DOOGIE_PAGE_TREE_ITEM_H_
#define DOOGIE_PAGE_TREE_ITEM_H_

#include <QtWidgets>
#include "browser_widget.h"

namespace doogie {

class PageTreeItem : public QTreeWidgetItem {
 public:
  explicit PageTreeItem(QPointer<BrowserWidget> browser);
  ~PageTreeItem();
  QPointer<BrowserWidget> Browser();
  QToolButton* CloseButton();
  void AfterAdded();

  PageTreeItem* Parent() { return static_cast<PageTreeItem*>(parent()); }

  QJsonObject DebugDump();

  bool SelfOrAnyChildCollapsed();
  bool SelfOrAnyChildExpanded();
  void ExpandSelfAndChildren();
  void CollapseSelfAndChildren();
  QList<PageTreeItem*> SelfSelectedOrChildrenSelected();
  bool SelectedOrHasSelectedParent();

  QList<PageTreeItem*> Siblings();

 private:
  void ApplyFavicon();

  QPointer<BrowserWidget> browser_;
  QToolButton* close_button_ = nullptr;
  QMetaObject::Connection loading_icon_frame_conn_;
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_TREE_ITEM_H_
