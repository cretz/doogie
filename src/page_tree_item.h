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
  QToolButton* close_button_;
};

#endif // DOOGIE_PAGETREEITEM_H_
