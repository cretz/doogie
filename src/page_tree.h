#ifndef DOOGIE_PAGETREE_H_
#define DOOGIE_PAGETREE_H_

#include <QtWidgets>
#include "browser_stack.h"

class PageTree : public QDockWidget {
  Q_OBJECT
 public:
  PageTree(BrowserStack *browser_stack, QWidget *parent = nullptr);

 private:
  BrowserStack *browser_stack_;
};

#endif // DOOGIE_PAGETREE_H_
