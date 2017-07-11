#ifndef DOOGIE_PAGETREEITEM_H_
#define DOOGIE_PAGETREEITEM_H_

#include <QtWidgets>
#include "browser_widget.h"

class PageTreeItem : public QTreeWidgetItem {
 public:
  explicit PageTreeItem(QPointer<BrowserWidget> browser);
  ~PageTreeItem();
  QPointer<BrowserWidget> Browser();
  QToolButton* CloseButton();
  void AfterAdded();
 private:
  QPointer<BrowserWidget> browser_;
  QToolButton* close_button_ = nullptr;
  QMetaObject::Connection loading_icon_frame_conn_;

  void ApplyFavicon();
};

#endif // DOOGIE_PAGETREEITEM_H_
