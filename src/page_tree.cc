#include "page_tree.h"
#include <algorithm>

namespace doogie {

PageTree::PageTree(BrowserStack* browser_stack, QWidget* parent)
    : QTreeWidget(parent), browser_stack_(browser_stack) {
  setColumnCount(2);
  setHeaderHidden(true);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setDragDropMode(QAbstractItemView::DragDrop);
  setDefaultDropAction(Qt::MoveAction);
  setDragEnabled(true);
  viewport()->setAcceptDrops(true);
  setDropIndicatorShown(true);
  setAutoExpandDelay(500);
  setIndentation(10);
  setAnimated(false);
  header()->setStretchLastSection(false);
  header()->setSectionResizeMode(0, QHeaderView::Stretch);
  header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

  // Each time one is selected, we need to make sure to show that on the stack
  connect(this, &QTreeWidget::currentItemChanged,
          [this](QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    // Deactivate previous
    if (previous) {
      auto old_font = previous->font(0);
      old_font.setBold(false);
      previous->setFont(0, old_font);
    }
    // Sometimes current isn't set or isn't in the tree anymore
    if (current && indexFromItem(current).isValid()) {
      auto page_item = static_cast<PageTreeItem*>(current);
      browser_stack_->setCurrentWidget(page_item->Browser());
      auto new_font = page_item->font(0);
      new_font.setBold(true);
      page_item->setFont(0, new_font);
    }
  });

  // Connect the simple close
  connect(this, &PageTree::ItemClose, this, &PageTree::CloseItem);

  connect(this, &PageTree::ItemCloseRelease, [this](PageTreeItem* item) {
    auto button_down = QApplication::mouseButtons().testFlag(Qt::LeftButton);
    if (button_down) {
      item->CloseButton()->setDown(true);

      // First drag?
      if (!close_dragging_) {
        close_dragging_ = true;
        close_dragging_on_ = item;
        item->CloseButton()->setChecked(true);
      }

      // We are dragging, grab the one under the mouse
      auto local_pos = mapFromGlobal(QCursor::pos());
      auto mouse_item = static_cast<PageTreeItem*>(itemAt(local_pos));
      // Only applies if there is a close button under the mouse. It also
      // can't be the last one we saw being dragged on
      if (mouse_item && mouse_item != close_dragging_on_ &&
          columnAt(local_pos.x()) == 1) {
        // Flip the checked state
        mouse_item->CloseButton()->setChecked(
              !mouse_item->CloseButton()->isChecked());
        close_dragging_on_ = mouse_item;
      }
    }
  });
}

void PageTree::NewPage(const QString& url, bool top_level) {
  auto browser = browser_stack_->NewBrowser(url);
  auto parent = (top_level) ?
        nullptr : static_cast<PageTreeItem*>(currentItem());
  AddBrowser(browser, parent, true);
  browser->FocusUrlEdit();
}

void PageTree::CloseCurrentPage() {
  auto current_item = static_cast<PageTreeItem*>(currentItem());
  if (current_item) {
    // We always expand here if there are child items so
    // that we don't close those too
    if (current_item->childCount() > 0) current_item->setExpanded(true);
    CloseItem(current_item);
  }
}

void PageTree::CloseAllPages() {
  // Get persistent indices for everything, then close em all
  QTreeWidgetItemIterator it(this);
  QList<QPersistentModelIndex> to_close;
  while (*it) {
    // Prepend to do children first
    to_close.prepend(indexFromItem(*it));
    it++;
  }
  // Now try to close each one
  for (const auto &index : to_close) {
    auto tree_item = static_cast<PageTreeItem*>(itemFromIndex(index));
    if (tree_item) CloseItem(tree_item);
  }
}

QMovie* PageTree::LoadingIconMovie() {
  if (!loading_icon_movie_) {
    loading_icon_movie_ = new QMovie(":/res/images/loading-icon.gif");
    // We use a bad method to update the icon on frames of this,
    // so slow it down.
    loading_icon_movie_->setSpeed(70);
  }
  return loading_icon_movie_;
}

QJsonObject PageTree::DebugDump() {
  QJsonArray items;
  for (int i = 0; i < topLevelItemCount(); i++) {
    items.append(static_cast<PageTreeItem*>(topLevelItem(i))->DebugDump());
  }
  return  {
    { "items", items }
  };
}

Qt::DropActions PageTree::supportedDropActions() const {
  // return Qt::MoveAction | Qt::TargetMoveAction;
  return QTreeWidget::supportedDropActions();
}

void PageTree::dragEnterEvent(QDragEnterEvent* event) {
  // We accept external types of URL
  if (event->source() != this && event->mimeData()->hasUrls()) {
    if (event->proposedAction() != Qt::CopyAction) {
      event->ignore();
      return;
    }
    event->acceptProposedAction();
  }
  QTreeWidget::dragEnterEvent(event);
}

void PageTree::dragMoveEvent(QDragMoveEvent* event) {
  if (event->source() != this && event->mimeData()->hasUrls()) {
    if (event->proposedAction() != Qt::CopyAction) {
      event->ignore();
      return;
    }
    event->acceptProposedAction();
  }
  QTreeWidget::dragMoveEvent(event);
}

void PageTree::dropEvent(QDropEvent* event) {
  // Due to bad internal Qt logic, we reset the current here
  if (event->source() == this) {
    auto current = currentItem();
    QTreeWidget::dropEvent(event);
    setCurrentItem(current, 0, QItemSelectionModel::NoUpdate);
  } else {
    if (event->mimeData()->hasUrls() &&
        event->proposedAction() != Qt::CopyAction) {
      event->ignore();
      return;
    }
    QTreeWidget::dropEvent(event);
  }
}

bool PageTree::dropMimeData(QTreeWidgetItem* parent,
                            int index,
                            const QMimeData* data,
                            Qt::DropAction action) {
  // If there is a URL we go ahead and put each one under the parent
  if (data->hasUrls()) {
    for (const auto& url : data->urls()) {
      AddBrowser(browser_stack_->NewBrowser(url.url()),
                 static_cast<PageTreeItem*>(parent),
                 true);
    }
    return true;
  }
  return QTreeWidget::dropMimeData(parent, index, data, action);
}

void PageTree::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Delete) {
    if (!event->isAutoRepeat()) {
      // Close all selected items. We use the iterator instead of
      // selectedIndexes so we are ordered
      QList<QPersistentModelIndex> to_close;
      QTreeWidgetItemIterator it(this);
      while (*it) {
        if ((*it)->isSelected()) {
          // Prepend to do children first
          to_close.prepend(indexFromItem(*it));
        }
        it++;
      }
      for (const auto &index : to_close) {
        auto tree_item = static_cast<PageTreeItem*>(itemFromIndex(index));
        if (tree_item) CloseItem(tree_item);
      }
    }
  } else {
    QTreeWidget::keyPressEvent(event);
  }
}

QMimeData* PageTree::mimeData(const QList<QTreeWidgetItem*> items) const {
  auto ret = QTreeWidget::mimeData(items);
  QList<QUrl> urls;
  for (const auto& item : items) {
    auto browser = static_cast<PageTreeItem*>(item)->Browser();
    if (browser) {
      auto url = browser->CurrentUrl();
      if (!url.isEmpty()) {
        urls.append(QUrl(url));
      }
    }
  }
  if (!urls.isEmpty()) {
    ret->setUrls(urls);
  }
  return ret;
}

QStringList PageTree::mimeTypes() const {
  return { "text/uri-list" };
}

void PageTree::mouseDoubleClickEvent(QMouseEvent *event) {
  if (itemAt(event->pos())) {
    QTreeWidget::mouseDoubleClickEvent(event);
  } else {
    NewPage("",
            !QApplication::keyboardModifiers().testFlag(Qt::ControlModifier));
  }
}

void PageTree::mousePressEvent(QMouseEvent* event) {
  // We start a rubber band selection if left of the tree or if there is no
  // item where we pressed.
  auto item = itemAt(event->pos());
  if (event->x() <= indentation() || !item) {
    // We'll clear the select if control is not pressed
    if (!QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
      clearSelection();
      rubber_band_orig_selected_ = QItemSelection();
    } else {
      rubber_band_orig_selected_ = selectionModel()->selection();
    }
    rubber_band_origin_ = event->pos();
    if (!rubber_band_) {
      rubber_band_ = new QRubberBand(QRubberBand::Rectangle, this);
    }
    rubber_band_->setGeometry(QRect(rubber_band_origin_, QSize()));
    rubber_band_->show();
    // As a special case, if the item doesn't have children and the click
    // was in the indentation, we don't want to continue because it starts
    // a drag. But we do want to continue if it has children because that's
    // where the expander button is.
    if (event->x() <= indentation() && item && item->childCount() == 0) {
      return;
    }
  }
  QTreeWidget::mousePressEvent(event);
}

void PageTree::mouseMoveEvent(QMouseEvent* event) {
  // If we are rubber band selecting, keep using that
  if (rubber_band_ && !rubber_band_->isHidden()) {
    // Restore the selection we knew
    selectionModel()->select(rubber_band_orig_selected_,
                             QItemSelectionModel::ClearAndSelect);
    // Basically, we are just going to take a peek at the top and then work
    // downwards Selecting the first column.
    auto rect = QRect(rubber_band_origin_, event->pos()).normalized();
    auto index = indexAt(QPoint(0, std::max(0, rect.top())));
    while (index.isValid()) {
      // We toggle here so during ctrl+drag they can keep the originals
      selectionModel()->select(index, QItemSelectionModel::Toggle);
      index = indexBelow(index);
      if (index.isValid() && visualRect(index).y() > rect.bottom()) {
        index = QModelIndex();
      }
    }
    rubber_band_->setGeometry(QRect(rubber_band_origin_,
                                    event->pos()).normalized());
  } else if (state() != DragSelectingState) {
    // Prevent the drag-select
    QTreeWidget::mouseMoveEvent(event);
  }
}

void PageTree::mouseReleaseEvent(QMouseEvent* event) {
  // End rubber band selection
  if (rubber_band_ && !rubber_band_->isHidden()) {
    rubber_band_->hide();
  } else {
    QTreeWidget::mouseReleaseEvent(event);
    if (close_dragging_) {
      // Close-button drag has completed, close what's checked
      close_dragging_on_ = nullptr;
      close_dragging_ = false;
      // Obtain a list of what to close
      QTreeWidgetItemIterator it(this);
      QList<QPersistentModelIndex> to_close;
      while (*it) {
        auto tree_item = static_cast<PageTreeItem*>(*it);
        if (tree_item->CloseButton()->isChecked()) {
          // Prepend to close children first
          to_close.prepend(indexFromItem(tree_item));
        }
        ++it;
      }
      // Now try to close each one
      for (const auto &index : to_close) {
        CloseItem(static_cast<PageTreeItem*>(itemFromIndex(index)));
      }
    }
  }
}

void PageTree::rowsInserted(const QModelIndex& parent, int start, int end) {
  // We have to override this to re-add the child widget due to how row
  // movement occurs.
  // Ref: https://stackoverflow.com/questions/25559221/qtreewidgetitem-issue-items-set-using-setwidgetitem-are-dispearring-after-movin
  for (int i = start; i <= end; i++) {
    auto item = static_cast<PageTreeItem*>(
          itemFromIndex(model()->index(i, 0, parent)));
    item->AfterAdded();
  }
  QTreeWidget::rowsInserted(parent, start, end);
}

void PageTree::AddBrowser(QPointer<BrowserWidget> browser,
                          PageTreeItem* parent,
                          bool make_current) {
  // Create the tree item
  auto browser_item = new PageTreeItem(browser);

  // Put as top level or as child
  if (parent) {
    parent->addChild(browser_item);
    // If the child is the first child, we expand automatically
    if (parent->childCount() == 1) {
      parent->setExpanded(true);
    }
  } else {
    addTopLevelItem(browser_item);
  }
  // Whether to set it as the current active
  if (make_current) {
    setCurrentItem(browser_item, 0, QItemSelectionModel::Current);
  }
  // Make all tab opens open as child
  connect(browser, &BrowserWidget::PageOpen,
          [this, browser_item](BrowserWidget::WindowOpenType type,
                               const QString& url,
                               bool user_gesture) {
    PageTreeItem* parent = nullptr;
    bool make_current = user_gesture &&
        type != BrowserWidget::OpenTypeNewBackgroundTab;
    if (type != BrowserWidget::OpenTypeNewWindow) {
      parent = browser_item;
    }
    AddBrowser(browser_stack_->NewBrowser(url), parent, make_current);
    if (make_current) browser_item->Browser()->FocusBrowser();
  });
}

void PageTree::CloseItem(PageTreeItem* item) {
  // Eagerly skip this if the item ain't a thing anymore
  if (!item) return;
  // We only close children if we're not expanded.
  if (!item->isExpanded()) {
    // Close backwards up the list
    for (int i = item->childCount() - 1; i >= 0; i--) {
      CloseItem(static_cast<PageTreeItem*>(item->child(i)));
    }
  }
  // Now we can close myself
  item->Browser()->TryClose();
}

}  // namespace doogie
