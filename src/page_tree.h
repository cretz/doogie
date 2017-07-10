#ifndef DOOGIE_PAGETREE_H_
#define DOOGIE_PAGETREE_H_

#include <QtWidgets>
#include "browser_stack.h"
#include "browser_widget.h"
#include "page_tree_item.h"

class PageTree : public QTreeWidget {
  Q_OBJECT
 public:
  explicit PageTree(BrowserStack *browser_stack, QWidget *parent = nullptr);

  void NewBrowser();
 protected:
  virtual Qt::DropActions supportedDropActions() const override;
  virtual void dropEvent(QDropEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void rowsInserted(const QModelIndex &parent,
                            int start,
                            int end) override;
 private:
  BrowserStack *browser_stack_;

  void AddBrowser(QPointer<BrowserWidget> browser,
                  PageTreeItem *parent,
                  bool make_current);
 signals:
  void ItemClose(PageTreeItem *item);
};

#endif // DOOGIE_PAGETREE_H_
