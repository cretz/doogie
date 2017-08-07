#ifndef DOOGIE_PAGE_TREE_ITEM_H_
#define DOOGIE_PAGE_TREE_ITEM_H_

#include <QtWidgets>

#include "browser_widget.h"

namespace doogie {

class PageTreeItem : public QTreeWidgetItem {
 public:
  static const int kBubbleIconColumn = 1;
  static const int kCloseButtonColumn = 2;

  explicit PageTreeItem(QPointer<BrowserWidget> browser);
  ~PageTreeItem();
  QPointer<BrowserWidget> Browser() const;
  QToolButton* CloseButton() const;
  void AfterAdded();

  PageTreeItem* Parent() const { return static_cast<PageTreeItem*>(parent()); }

  QJsonObject DebugDump() const;

  bool SelfOrAnyChildCollapsed() const;
  bool SelfOrAnyChildExpanded() const;
  void ExpandSelfAndChildren();
  void CollapseSelfAndChildren();
  QList<PageTreeItem*> SelfSelectedOrChildrenSelected() const;
  bool SelectedOrHasSelectedParent() const;

  QList<PageTreeItem*> Siblings() const;

  void SetCurrentBubbleIfDifferent(const Bubble& bubble);

 private:
  void ApplyFavicon();

  QPointer<BrowserWidget> browser_;
  QToolButton* close_button_ = nullptr;
  QMetaObject::Connection loading_icon_frame_conn_;
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_TREE_ITEM_H_
