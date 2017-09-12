#ifndef DOOGIE_PAGE_TREE_DOCK_H_
#define DOOGIE_PAGE_TREE_DOCK_H_

#include <QtWidgets>

#include "page_tree.h"

namespace doogie {

// Dock window housing the page tree.
class PageTreeDock : public QDockWidget {
  Q_OBJECT

 public:
  explicit PageTreeDock(BrowserStack* browser_stack,
                        QWidget* parent = nullptr);

  void NewPage(const QString &url, bool top_level, bool make_current);
  bool HasTopLevelItems() const;

  void CloseAllWorkspaces();

  QJsonObject DebugDump() const;

 signals:
  void TreeEmpty();

 private:
  PageTree* tree_;
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_TREE_DOCK_H_
