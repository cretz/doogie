#ifndef DOOGIE_PAGE_TREE_H_
#define DOOGIE_PAGE_TREE_H_

#include <QtWidgets>
#include "browser_stack.h"
#include "browser_widget.h"
#include "page_tree_item.h"

namespace doogie {

class PageTree : public QTreeWidget {
  Q_OBJECT

 public:
  explicit PageTree(BrowserStack* browser_stack, QWidget* parent = nullptr);
  void NewPage(const QString& url, bool top_level);
  void CloseCurrentPage();
  void CloseAllPages();
  QMovie* LoadingIconMovie();

  QJsonObject DebugDump();

 protected:
  Qt::DropActions supportedDropActions() const override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
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
  PageTreeItem* AddBrowser(QPointer<BrowserWidget> browser,
                           PageTreeItem* parent,
                           bool make_current);
  void CloseItem(PageTreeItem* item);
  void DuplicateTree(PageTreeItem* item, PageTreeItem* to_parent);
  QList<PageTreeItem*> Items();
  QList<PageTreeItem*> ItemsInReverse();
  QList<PageTreeItem*> SelectedItems();
  QList<PageTreeItem*> SelectedItemsInReverse();
  QList<PageTreeItem*> SelectedItemsOnlyHighestLevel();

  BrowserStack* browser_stack_ = nullptr;
  QMovie* loading_icon_movie_ = nullptr;
  bool close_dragging_ = false;
  PageTreeItem* close_dragging_on_ = nullptr;
  QRubberBand* rubber_band_ = nullptr;
  QPoint rubber_band_origin_;
  QItemSelection rubber_band_orig_selected_;

 signals:
  void ItemClose(PageTreeItem* item);
  void ItemClosePress(PageTreeItem* item);
  void ItemCloseRelease(PageTreeItem* item);
  void ItemDestroyed(PageTreeItem* item);
  void TreeEmpty();
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_TREE_H_
