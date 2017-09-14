#ifndef DOOGIE_PAGE_TREE_H_
#define DOOGIE_PAGE_TREE_H_

#include <QtWidgets>

#include "browser_stack.h"
#include "browser_widget.h"
#include "page_tree_item.h"
#include "workspace.h"
#include "workspace_tree_item.h"

namespace doogie {

// The tree of pages.
class PageTree : public QTreeWidget {
  Q_OBJECT

 public:
  static const int kPageItemType = QTreeWidgetItem::UserType + 1;
  static const int kWorkspaceItemType = QTreeWidgetItem::UserType + 2;

  explicit PageTree(BrowserStack* browser_stack, QWidget* parent = nullptr);
  QMovie* LoadingIconMovie();
  PageTreeItem* CurrentItem() const;
  PageTreeItem* NewPage(const QString& url,
                        PageTreeItem* parent,
                        bool make_current);
  PageTreeItem* NewPage(Workspace::WorkspacePage* page,
                        PageTreeItem* parent,
                        bool make_current);
  void ApplyBubbleSelectMenu(QMenu* menu,
                             QList<PageTreeItem*> apply_to_items);

  void ApplyRecentWorkspacesMenu(QMenu* menu);
  void ApplyWorkspaceMenu(QMenu* menu,
                          const Workspace& workspace,
                          WorkspaceTreeItem* item);
  WorkspaceTreeItem* OpenWorkspace(Workspace* workspace);

  const Workspace& ImplicitWorkspace() const { return implicit_workspace_; }
  bool HasImplicitWorkspace() const { return has_implicit_workspace_; }

  // Does not persist anything about the close, expected to be
  // called on app close
  void CloseAllWorkspaces();

  void WorkspaceAboutToDestroy(WorkspaceTreeItem* item);

  void SetCurrentClosestTo(QTreeWidgetItem* item);

  void EditWorkspaceName(WorkspaceTreeItem* item);

  QJsonObject DebugDump() const;

 protected:
  Qt::DropActions supportedDropActions() const override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void drawRow(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
  void dropEvent(QDropEvent* event) override;
  bool dropMimeData(QTreeWidgetItem* parent,
                    int index, const QMimeData* data,
                    Qt::DropAction action) override;
  void keyPressEvent(QKeyEvent* event) override;
  QMimeData* mimeData(const QList<QTreeWidgetItem*> items) const override;
  QStringList mimeTypes() const override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void rowsInserted(const QModelIndex& parent,
                    int start,
                    int end) override;
  QItemSelectionModel::SelectionFlags selectionCommand(
      const QModelIndex& index, const QEvent* event) const override;

 private:
  PageTreeItem* AsPageTreeItem(QTreeWidgetItem* item) const;
  WorkspaceTreeItem* AsWorkspaceTreeItem(QTreeWidgetItem* item) const;
  void SetupActions();
  void SetupInitialWorkspaces();
  PageTreeItem* AddBrowser(QPointer<BrowserWidget> browser,
                           Workspace::WorkspacePage* page,
                           PageTreeItem* parent,
                           bool make_current);
  void CloseWorkspace(WorkspaceTreeItem* item, bool send_close_event = true);
  void CloseItem(PageTreeItem* item,
                 bool workspace_persist = true,
                 bool force_close_children = false);
  void CloseItemsInReverseOrder(QList<PageTreeItem*> items,
                                bool workspace_persist = true);
  void DuplicateTree(PageTreeItem* item, PageTreeItem* to_parent = nullptr);
  QList<PageTreeItem*> Items();
  QList<PageTreeItem*> SelectedItems();
  QList<PageTreeItem*> SelectedItemsOnlyHighestLevel();
  QList<PageTreeItem*> SameHostPages(PageTreeItem* to_comp);
  QList<Workspace> Workspaces() const;
  QList<qlonglong> WorkspaceIds() const;
  const Workspace& WorkspaceToAddUnder() const;
  WorkspaceTreeItem* WorkspaceTreeItemToAddUnder() const;
  void MakeWorkspaceExplicitIfPossible();
  void MakeWorkspaceImplicitIfPossible();

  BrowserStack* browser_stack_ = nullptr;
  QMovie* loading_icon_movie_ = nullptr;
  QRubberBand* rubber_band_ = nullptr;
  QPoint rubber_band_origin_;
  QItemSelection rubber_band_orig_selected_;

  Workspace implicit_workspace_;
  bool has_implicit_workspace_ = false;

 signals:
  void ItemCloseRelease();
  void ItemDestroyed(PageTreeItem* item);
  void TreeEmpty();
  void WorkspaceImplicitnessChanged();
  void WorkspaceClosed(qlonglong id);
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_TREE_H_
