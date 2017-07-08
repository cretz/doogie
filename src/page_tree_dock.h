#ifndef DOOGIE_PAGETREEDOCK_H_
#define DOOGIE_PAGETREEDOCK_H_

#include <QtWidgets>
#include "page_tree.h"

class PageTreeDock : public QDockWidget {
  Q_OBJECT
 public:
  PageTreeDock(BrowserStack *browser_stack, QWidget *parent = nullptr);

 private:
  PageTree *tree_;
};

#endif // DOOGIE_PAGETREEDOCK_H_
