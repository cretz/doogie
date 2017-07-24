#ifndef DOOGIE_PAGE_TREE_DOCK_H_
#define DOOGIE_PAGE_TREE_DOCK_H_

#include <QtWidgets>
#include "page_tree.h"

namespace doogie {

class PageTreeDock : public QDockWidget {
  Q_OBJECT

 public:
  explicit PageTreeDock(BrowserStack* browser_stack,
                        QWidget* parent = nullptr);

  void NewPage(const QString &url, bool top_level, bool make_current);
  bool HasOpenPages();

  QJsonObject DebugDump();

 private:
  PageTree* tree_;

 signals:
  void TreeEmpty();
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_TREE_DOCK_H_
