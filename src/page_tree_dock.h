#ifndef DOOGIE_PAGE_TREE_DOCK_H_
#define DOOGIE_PAGE_TREE_DOCK_H_

#include <QtWidgets>
#include "page_tree.h"

namespace doogie {

class PageTreeDock : public QDockWidget {
  Q_OBJECT

 public:
  PageTreeDock(BrowserStack* browser_stack, QWidget* parent = nullptr);

  void FocusPageTree();
  void NewTopLevelPage(const QString& url);
  void NewChildPage(const QString& url);
  void CloseCurrentPage();
  void CloseAllPages();

 private:
  PageTree* tree_;
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_TREE_DOCK_H_
