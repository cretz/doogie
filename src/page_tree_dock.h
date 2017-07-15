#ifndef DOOGIE_PAGETREEDOCK_H_
#define DOOGIE_PAGETREEDOCK_H_

#include <QtWidgets>
#include "page_tree.h"

class PageTreeDock : public QDockWidget {
  Q_OBJECT
 public:
  PageTreeDock(BrowserStack *browser_stack, QWidget *parent = nullptr);

  void FocusPageTree();
  void NewTopLevelPage(const QString &url);
  void NewChildPage(const QString &url);
  void CloseCurrentPage();
  void CloseAllPages();

 private:
  PageTree *tree_;
};

#endif // DOOGIE_PAGETREEDOCK_H_
