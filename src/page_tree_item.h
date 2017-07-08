#ifndef DOOGIE_PAGETREEITEM_H_
#define DOOGIE_PAGETREEITEM_H_

#include <QtWidgets>
#include "browser_widget.h"

class PageTreeItem : public QTreeWidgetItem {
 public:
  explicit PageTreeItem(QPointer<BrowserWidget> browser);
  QPointer<BrowserWidget> Browser();
 private:
  QPointer<BrowserWidget> browser_;
};

#endif // DOOGIE_PAGETREEITEM_H_
