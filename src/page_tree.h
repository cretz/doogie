#ifndef DOOGIE_PAGETREE_H_
#define DOOGIE_PAGETREE_H_

#include <QtWidgets>
#include "browser_stack.h"
#include "page_tree_item.h"

class PageTree : public QDockWidget {
  Q_OBJECT
 public:
  PageTree(BrowserStack *browser_stack, QWidget *parent = nullptr);

 private:
  BrowserStack *browser_stack_;
  QTreeWidget *tree_;
  PageTreeItem *active_item_;

  void AddBrowser(QPointer<BrowserWidget> browser,
                  PageTreeItem *parent,
                  bool make_current);
};

#endif // DOOGIE_PAGETREE_H_
