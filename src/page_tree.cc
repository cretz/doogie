#include "page_tree.h"
#include <algorithm>
#include "action_manager.h"
#include "bubble_settings_dialog.h"
#include "workspace_tree_item.h"

namespace doogie {

PageTree::PageTree(BrowserStack* browser_stack, QWidget* parent)
    : QTreeWidget(parent), browser_stack_(browser_stack) {
  setColumnCount(PageTreeItem::kCloseButtonColumn + 1);
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
  setColumnWidth(PageTreeItem::kBubbleIconColumn, 16);
  header()->setSectionResizeMode(PageTreeItem::kBubbleIconColumn,
                                 QHeaderView::Fixed);
  header()->setSectionResizeMode(PageTreeItem::kCloseButtonColumn,
                                 QHeaderView::ResizeToContents);
  setStyleSheet("QTreeWidget { border: none; }");

  // Emit empty on row removal
  connect(model(), &QAbstractItemModel::rowsRemoved,
          [this](const QModelIndex&, int, int) {
    if (topLevelItemCount() == 0) emit TreeEmpty();
  });

  // Each time one is selected, we need to make sure to show that on the stack
  connect(this, &QTreeWidget::currentItemChanged,
          [this](QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    // Before anything, if current is a workspace item, we don't support
    //  "current" so put it back
    if (current && current->type() == kWorkspaceItemType) {
      if (!previous || previous->type() != kWorkspaceItemType) {
        setCurrentItem(previous);
      } else {
        setCurrentItem(nullptr);
      }
      return;
    }
    // Deactivate previous
    if (previous) {
      auto old_font = previous->font(0);
      old_font.setBold(false);
      previous->setFont(0, old_font);
    }
    // Sometimes current isn't set or isn't in the tree anymore
    if (current && indexFromItem(current).isValid()) {
      auto page_item = AsPageTreeItem(current);
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
      auto mouse_item = AsPageTreeItem(itemAt(local_pos));
      // Only applies if there is a close button under the mouse. It also
      // can't be the last one we saw being dragged on
      if (mouse_item && mouse_item != close_dragging_on_ &&
          columnAt(local_pos.x()) == PageTreeItem::kCloseButtonColumn) {
        // Flip the checked state
        mouse_item->CloseButton()->setChecked(
              !mouse_item->CloseButton()->isChecked());
        close_dragging_on_ = mouse_item;
      }
    }
  });

  SetupActions();
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

PageTreeItem* PageTree::CurrentItem()  {
  return AsPageTreeItem(currentItem());
}

PageTreeItem* PageTree::NewPage(const QString &url,
                                PageTreeItem* parent,
                                bool make_current) {
  Bubble* bubble;
  if (parent) {
    bubble = parent->Browser()->CurrentBubble();
  } else {
    bubble = Profile::Current()->DefaultBubble();
  }
  auto browser = browser_stack_->NewBrowser(bubble, url);
  return AddBrowser(browser, parent, make_current);
}

void PageTree::ApplyBubbleSelectMenu(QMenu* menu,
                                     QList<PageTreeItem*> apply_to_items) {
  // If there is a commonly selected one for all of them, we will mark
  // that one as checked and unable to be selected
  QString common_bubble_name;
  for (const auto& item : apply_to_items) {
    auto bubble_name = item->Browser()->CurrentBubble()->Name();
    if (common_bubble_name.isNull()) {
      common_bubble_name = bubble_name;
    } else if (common_bubble_name != bubble_name) {
      common_bubble_name = QString();
      break;
    }
  }
  for (const auto& bubble : Profile::Current()->Bubbles()) {
    auto action = menu->addAction(bubble->Icon(), bubble->FriendlyName());
    if (!common_bubble_name.isNull() && common_bubble_name == bubble->Name()) {
      action->setCheckable(true);
      action->setChecked(true);
      action->setDisabled(true);
    } else {
      connect(action, &QAction::triggered, [=]() {
        for (const auto& item : apply_to_items) {
          item->SetCurrentBubbleIfDifferent(bubble);
        }
      });
    }
  }
  menu->addSeparator();
  menu->addAction("New Bubble...", [=]() {
    auto bubble = BubbleSettingsDialog::NewBubble();
    if (bubble) {
      for (const auto& item : apply_to_items) {
        item->SetCurrentBubbleIfDifferent(bubble);
      }
    }
  });
}

QJsonObject PageTree::DebugDump() {
  QJsonArray items;
  for (int i = 0; i < topLevelItemCount(); i++) {
    auto item = AsPageTreeItem(topLevelItem(i));
    items.append(item->DebugDump());
  }
  return  {
    { "items", items }
  };
}

Qt::DropActions PageTree::supportedDropActions() const {
  // return Qt::MoveAction | Qt::TargetMoveAction;
  return QTreeWidget::supportedDropActions();
}

void PageTree::contextMenuEvent(QContextMenuEvent* event) {
  QMenu menu;

  menu.addAction(ActionManager::Action(ActionManager::NewTopLevelPage));

  // Single-page
  auto affected = AsPageTreeItem(itemAt(event->pos()));
  if (affected) {
    auto sub = menu.addMenu("Clicked-On Page");
    sub->addAction("New Child Background Page", [=]() {
      NewPage("", affected, false);
    });
    sub->addAction("New Foreground Child Page", [=]() {
      auto page = NewPage("", affected, false);
      page->Browser()->FocusUrlEdit();
    });
    sub->addAction("Reload Page", [=]() {
      affected->Browser()->Refresh();
    });
    sub->addAction("Suspend Page", [=]() {
      affected->Browser()->SetSuspended(true);
    })->setEnabled(!affected->Browser()->Suspended());
    sub->addAction("Unsuspend Page", [=]() {
      affected->Browser()->SetSuspended(false);
    })->setEnabled(affected->Browser()->Suspended());
    sub->addAction("Expand Tree", [=]() {
      affected->ExpandSelfAndChildren();
    })->setEnabled(affected->SelfOrAnyChildCollapsed());
    sub->addAction("Collapse Tree", [=]() {
      affected->CollapseSelfAndChildren();
    })->setEnabled(affected->SelfOrAnyChildExpanded());
    sub->addAction("Duplicate Tree", [=]() {
      DuplicateTree(affected);
    });
    sub->addAction("Select Same-Host Pages", [=]() {
      clearSelection();
      for (const auto& item : SameHostPages(affected)) {
        item->setSelected(true);
      }
    });
    sub->addAction("Close Page", [=]() {
      // We expand this to force children upwards
      affected->setExpanded(true);
      CloseItem(affected);
    });
    sub->addAction("Close Tree", [=]() {
      // We collapse everything here to force children to close
      affected->CollapseSelfAndChildren();
      CloseItem(affected);
    });
    sub->addAction("Close Same-Host Pages", [=]() {
      CloseItemsInReverseOrder(SameHostPages(affected));
    });
    sub->addAction("Close Other Trees", [=]() {
      CloseItemsInReverseOrder(affected->Siblings());
    });
    auto bubble_menu = menu.addMenu("Clicked-On Page Bubble");
    ApplyBubbleSelectMenu(bubble_menu, { affected });
  } else {
    affected = CurrentItem();
    if (affected) {
      auto sub = menu.addMenu("Current Page");
      sub->addAction(ActionManager::Action(
                      ActionManager::NewChildBackgroundPage));
      sub->addAction(ActionManager::Action(
                      ActionManager::NewChildForegroundPage));
      sub->addAction(ActionManager::Action(
                      ActionManager::Reload));
      sub->addAction(ActionManager::Action(
                      ActionManager::SuspendPage));
      sub->addAction(ActionManager::Action(
                      ActionManager::UnsuspendPage));
      sub->addAction(ActionManager::Action(
                      ActionManager::ExpandTree));
      sub->addAction(ActionManager::Action(
                      ActionManager::CollapseTree));
      sub->addAction(ActionManager::Action(
                      ActionManager::DuplicateTree));
      sub->addAction(ActionManager::Action(
                      ActionManager::SelectSameHostPages));
      sub->addAction(ActionManager::Action(
                      ActionManager::ClosePage));
      sub->addAction(ActionManager::Action(
                      ActionManager::CloseTree));
      sub->addAction(ActionManager::Action(
                      ActionManager::CloseSameHostPages));
      sub->addAction(ActionManager::Action(
                      ActionManager::CloseOtherTrees));
      auto bubble_menu = menu.addMenu("Current Page Bubble");
      ApplyBubbleSelectMenu(bubble_menu, { affected });
    }
  }

  // Selected pages
  auto selected_indices = selectedIndexes();
  auto has_selected_not_affected = selected_indices.count() > 1 ||
      (selected_indices.count() == 1 &&
        (!affected || selected_indices[0] != indexFromItem(affected)));
  if (has_selected_not_affected) {
    auto sub = menu.addMenu("Selected Pages");
    sub->addAction(ActionManager::Action(
        ActionManager::ReloadSelectedPages));
    sub->addAction(ActionManager::Action(
        ActionManager::SuspendSelectedPages));
    sub->addAction(ActionManager::Action(
        ActionManager::UnsuspendSelectedPages));
    sub->addAction(ActionManager::Action(
        ActionManager::ExpandSelectedTrees));
    sub->addAction(ActionManager::Action(
        ActionManager::CollapseSelectedTrees));
    sub->addAction(ActionManager::Action(
        ActionManager::DuplicateSelectedTrees));
    sub->addAction(ActionManager::Action(
        ActionManager::CloseSelectedPages));
    sub->addAction(ActionManager::Action(
        ActionManager::CloseSelectedTrees));
    sub->addAction(ActionManager::Action(
        ActionManager::CloseNonSelectedPages));
    sub->addAction(ActionManager::Action(
        ActionManager::CloseNonSelectedTrees));
    auto bubble_menu = menu.addMenu("Selected Pages Bubble");
    ApplyBubbleSelectMenu(bubble_menu, SelectedItems());
  }

  if (topLevelItemCount() > 0) {
    auto sub = menu.addMenu("All Pages");
    sub->addAction(ActionManager::Action(
        ActionManager::ReloadAllPages));
    sub->addAction(ActionManager::Action(
        ActionManager::SuspendAllPages));
    sub->addAction(ActionManager::Action(
        ActionManager::UnsuspendAllPages));
    sub->addAction(ActionManager::Action(
        ActionManager::ExpandAllTrees));
    sub->addAction(ActionManager::Action(
        ActionManager::CollapseAllTrees));
    sub->addAction(ActionManager::Action(
        ActionManager::CloseAllPages));
    auto bubble_menu = menu.addMenu("All Pages Bubble");
    ApplyBubbleSelectMenu(bubble_menu, Items());
  }

  menu.addAction("New Workspace", [=]() {
    auto item = new WorkspaceTreeItem();
    addTopLevelItem(item);
    editItem(item);
  });

  menu.exec(event->globalPos());
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

void PageTree::drawRow(QPainter* painter,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) const {
  if (itemFromIndex(index)->type() == kWorkspaceItemType) {
    painter->save();
    painter->fillRect(option.rect, QGuiApplication::palette().window());
    painter->restore();
  }
  QTreeWidget::drawRow(painter, option, index);
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
    auto parent_item = AsPageTreeItem(parent);
    for (const auto& url : data->urls()) {
      NewPage(url.url(), parent_item, true);
    }
    return true;
  }
  return QTreeWidget::dropMimeData(parent, index, data, action);
}

void PageTree::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Delete) {
    if (!event->isAutoRepeat()) {
      // Close all selected, no need to expand first
      CloseItemsInReverseOrder(SelectedItems());
    }
  } else {
    QTreeWidget::keyPressEvent(event);
  }
}

QMimeData* PageTree::mimeData(const QList<QTreeWidgetItem*> items) const {
  auto ret = QTreeWidget::mimeData(items);
  QList<QUrl> urls;
  for (const auto& item : items) {
    auto page_item = AsPageTreeItem(item);
    if (page_item) {
      auto browser = page_item->Browser();
      if (browser) {
        auto url = browser->CurrentUrl();
        if (!url.isEmpty()) urls.append(QUrl(url));
      }
    }
  }
  if (!urls.isEmpty()) ret->setUrls(urls);
  return ret;
}

QStringList PageTree::mimeTypes() const {
  return { "text/uri-list" };
}

void PageTree::mouseDoubleClickEvent(QMouseEvent *event) {
  if (itemAt(event->pos())) {
    QTreeWidget::mouseDoubleClickEvent(event);
  } else {
    PageTreeItem* parent = nullptr;
    if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
      parent = CurrentItem();
    }
    auto page = NewPage("", parent, true);
    page->Browser()->FocusUrlEdit();
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
  // Just ignore right buttons here to prevent "current" and select change
  if (event->button() != Qt::RightButton) {
    QTreeWidget::mousePressEvent(event);
  }
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
      // Close in reverse
      auto items = Items();
      for (auto i = items.crbegin(); i != items.crend(); i++) {
        if ((*i)->CloseButton()->isChecked()) {
          CloseItem(*i);
        }
      }
    }
  }
}

void PageTree::rowsInserted(const QModelIndex& parent, int start, int end) {
  // We have to override this to re-add the child widget due to how row
  // movement occurs.
  // Ref: https://stackoverflow.com/questions/25559221/qtreewidgetitem-issue-items-set-using-setwidgetitem-are-dispearring-after-movin
  for (int i = start; i <= end; i++) {
    auto item = itemFromIndex(model()->index(i, 0, parent));
    if (item && item->type() == kPageItemType) {
      AsPageTreeItem(item)->AfterAdded();
    } else if (item && item->type() == kWorkspaceItemType) {
      AsWorkspaceTreeItem(item)->AfterAdded();
    }
  }
  QTreeWidget::rowsInserted(parent, start, end);
}

QItemSelectionModel::SelectionFlags PageTree::selectionCommand(
    const QModelIndex& index, const QEvent* event) const {
  // If the event is a mouse press event and it's a right click,
  // we do not want to do any kind of update
  if (event && event->type() == QEvent::MouseButtonPress) {
    if (static_cast<const QMouseEvent*>(event)->button() == Qt::RightButton) {
      return QItemSelectionModel::NoUpdate;
    }
  }
  return QTreeWidget::selectionCommand(index, event);
}

PageTreeItem* PageTree::AsPageTreeItem(QTreeWidgetItem* item) const {
  if (item && item->type() == kPageItemType) {
    return static_cast<PageTreeItem*>(item);
  }
  return nullptr;
}

WorkspaceTreeItem* PageTree::AsWorkspaceTreeItem(QTreeWidgetItem* item) const {
  if (item && item->type() == kWorkspaceItemType) {
    return static_cast<WorkspaceTreeItem*>(item);
  }
  return nullptr;
}

void PageTree::SetupActions() {
  connect(ActionManager::Action(ActionManager::NewTopLevelPage),
          &QAction::triggered, [this]() {
    auto page = NewPage("", nullptr, true);
    // Focus URL edit
    page->Browser()->FocusUrlEdit();
  });
  connect(ActionManager::Action(ActionManager::NewChildForegroundPage),
          &QAction::triggered, [this]() {
    auto page = NewPage("", CurrentItem(), true);
    // Focus URL edit
    page->Browser()->FocusUrlEdit();
  });
  connect(ActionManager::Action(ActionManager::NewChildBackgroundPage),
          &QAction::triggered,
          [this]() { NewPage("", CurrentItem(), false); });
  connect(ActionManager::Action(ActionManager::Reload),
          &QAction::triggered,
          [this]() { CurrentItem()->Browser()->Refresh(); });
  connect(ActionManager::Action(ActionManager::SuspendPage),
          &QAction::triggered,
          [this]() { CurrentItem()->Browser()->SetSuspended(true); });
  connect(ActionManager::Action(ActionManager::UnsuspendPage),
          &QAction::triggered,
          [this]() { CurrentItem()->Browser()->SetSuspended(false); });
  connect(ActionManager::Action(ActionManager::Stop),
          &QAction::triggered,
          [this]() { CurrentItem()->Browser()->Stop(); });
  connect(ActionManager::Action(ActionManager::Forward),
          &QAction::triggered,
          [this]() { CurrentItem()->Browser()->Forward(); });
  connect(ActionManager::Action(ActionManager::Print),
          &QAction::triggered,
          [this]() { CurrentItem()->Browser()->Print(); });
  connect(ActionManager::Action(ActionManager::ZoomIn),
          &QAction::triggered, [this]() {
    CurrentItem()->Browser()->SetZoomLevel(
          CurrentItem()->Browser()->GetZoomLevel() + 0.1);
  });
  connect(ActionManager::Action(ActionManager::ZoomOut),
          &QAction::triggered, [this]() {
    CurrentItem()->Browser()->SetZoomLevel(
          CurrentItem()->Browser()->GetZoomLevel() - 0.1);
  });
  connect(ActionManager::Action(ActionManager::ResetZoom),
          &QAction::triggered,
          [this]() { CurrentItem()->Browser()->SetZoomLevel(0.0); });
  connect(ActionManager::Action(ActionManager::FindInPage),
          &QAction::triggered,
          [this]() { CurrentItem()->Browser()->ShowFind(); });
  connect(ActionManager::Action(ActionManager::ExpandTree),
          &QAction::triggered,
          [this]() { CurrentItem()->ExpandSelfAndChildren(); });
  connect(ActionManager::Action(ActionManager::CollapseTree),
          &QAction::triggered,
          [this]() { CurrentItem()->CollapseSelfAndChildren(); });
  connect(ActionManager::Action(ActionManager::DuplicateTree),
          &QAction::triggered,
          [this]() { DuplicateTree(CurrentItem()); });
  connect(ActionManager::Action(ActionManager::SelectSameHostPages),
          &QAction::triggered, [this]() {
    clearSelection();
    for (const auto& item : SameHostPages(CurrentItem())) {
      item->setSelected(true);
    }
  });
  connect(ActionManager::Action(ActionManager::ClosePage),
          &QAction::triggered, [this]() {
    // Expand it to prevent child close
    CurrentItem()->setExpanded(true);
    CloseItem(CurrentItem());
  });
  connect(ActionManager::Action(ActionManager::CloseTree),
          &QAction::triggered, [this]() {
    // Collapse to force child close
    CurrentItem()->CollapseSelfAndChildren();
    CloseItem(CurrentItem());
  });
  connect(ActionManager::Action(ActionManager::CloseSameHostPages),
          &QAction::triggered, [this]() {
    CloseItemsInReverseOrder(SameHostPages(CurrentItem()));
  });
  connect(ActionManager::Action(ActionManager::CloseOtherTrees),
          &QAction::triggered,
          [this]() { CloseItemsInReverseOrder(CurrentItem()->Siblings()); });

  connect(ActionManager::Action(ActionManager::ReloadSelectedPages),
          &QAction::triggered, [this]() {
    for (const auto& item : SelectedItems()) {
      item->Browser()->Refresh();
    }
  });
  connect(ActionManager::Action(ActionManager::SuspendSelectedPages),
          &QAction::triggered, [this]() {
    for (const auto& item : SelectedItems()) {
      item->Browser()->SetSuspended(true);
    }
  });
  connect(ActionManager::Action(ActionManager::UnsuspendSelectedPages),
          &QAction::triggered, [this]() {
    for (const auto& item : SelectedItems()) {
      item->Browser()->SetSuspended(false);
    }
  });
  connect(ActionManager::Action(ActionManager::ExpandSelectedTrees),
          &QAction::triggered, [this]() {
    for (const auto& item : SelectedItemsOnlyHighestLevel()) {
      item->ExpandSelfAndChildren();
    }
  });
  connect(ActionManager::Action(ActionManager::CollapseSelectedTrees),
          &QAction::triggered, [this]() {
    for (const auto& item : SelectedItemsOnlyHighestLevel()) {
      item->CollapseSelfAndChildren();
    }
  });
  // We accept that this ignores child selection if an ancestor
  // is selected. Otherwise, the definition of child+parent duplication
  // can be ambiguous depending on which you duplicate first.
  connect(ActionManager::Action(ActionManager::DuplicateSelectedTrees),
          &QAction::triggered, [this]() {
    for (const auto& item : SelectedItemsOnlyHighestLevel()) {
      DuplicateTree(item);
    }
  });
  connect(ActionManager::Action(ActionManager::CloseSelectedPages),
          &QAction::triggered, [this]() {
    // Go backwards and expand before closing
    auto items = SelectedItems();
    for (auto i = items.crbegin(); i != items.crend(); i++) {
      (*i)->setExpanded(true);
      CloseItem(*i);
    }
  });
  connect(ActionManager::Action(ActionManager::CloseSelectedTrees),
          &QAction::triggered, [this]() {
    for (const auto& item : SelectedItemsOnlyHighestLevel()) {
      // We always collapse to kill everything
      item->CollapseSelfAndChildren();
      CloseItem(item);
    }
  });
  connect(ActionManager::Action(ActionManager::CloseNonSelectedPages),
          &QAction::triggered, [this]() {
    // Expand non-selected and close in reverse
    auto items = Items();
    for (auto i = items.crbegin(); i != items.crend(); i++) {
      if (!(*i)->isSelected()) {
        (*i)->setExpanded(true);
        CloseItem(*i);
      }
    }
  });
  connect(ActionManager::Action(ActionManager::CloseNonSelectedTrees),
          &QAction::triggered, [this]() {
    // Basically go in reverse and expand+close anything not selected
    // and without a selected parent.
    auto items = Items();
    for (auto i = items.crbegin(); i != items.crend(); i++) {
      if (!(*i)->SelectedOrHasSelectedParent()) {
        (*i)->setExpanded(true);
        CloseItem(*i);
      }
    }
  });

  connect(ActionManager::Action(ActionManager::ReloadAllPages),
          &QAction::triggered, [this]() {
    for (const auto& item : Items()) {
      item->Browser()->Refresh();
    }
  });
  connect(ActionManager::Action(ActionManager::SuspendAllPages),
          &QAction::triggered, [this]() {
    for (const auto& item : Items()) {
      item->Browser()->SetSuspended(true);
    }
  });
  connect(ActionManager::Action(ActionManager::UnsuspendAllPages),
          &QAction::triggered, [this]() {
    for (const auto& item : Items()) {
      item->Browser()->SetSuspended(false);
    }
  });
  connect(ActionManager::Action(ActionManager::ExpandAllTrees),
          &QAction::triggered, [this]() {
    for (const auto& item : Items()) {
      item->setExpanded(true);
    }
  });
  connect(ActionManager::Action(ActionManager::CollapseAllTrees),
          &QAction::triggered, [this]() {
    for (const auto& item : Items()) {
      item->setExpanded(false);
    }
  });
  connect(ActionManager::Action(ActionManager::CloseAllPages),
          &QAction::triggered,
          [this]() { CloseItemsInReverseOrder(Items()); });
  connect(ActionManager::Action(ActionManager::FocusPageTree),
          &QAction::triggered,
          [this]() { setFocus(); });
}

PageTreeItem* PageTree::AddBrowser(QPointer<BrowserWidget> browser,
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
          [this, browser_item](CefHandler::WindowOpenType type,
                               const QString& url,
                               bool user_gesture) {
    PageTreeItem* parent = nullptr;
    // TODO: better UI and should I block all non-user page changes?
    if (!user_gesture) {
      qDebug() << "Popup blocked for: " << url;
      return;
    }
    bool make_current = user_gesture &&
        type != CefHandler::OpenTypeNewBackgroundTab;
    if (type != CefHandler::OpenTypeNewWindow) {
      parent = browser_item;
    }
    NewPage(url, parent, make_current);
    if (make_current) browser_item->Browser()->FocusBrowser();
  });
  return browser_item;
}

void PageTree::CloseItem(PageTreeItem* item) {
  // Eagerly skip this if the item ain't a thing anymore
  if (!item) return;
  // We only close children if we're not expanded.
  if (!item->isExpanded()) {
    // Close backwards up the list
    for (int i = item->childCount() - 1; i >= 0; i--) {
      CloseItem(AsPageTreeItem(item->child(i)));
    }
  }
  // Now we can close myself
  item->Browser()->TryClose();
}

void PageTree::CloseItemsInReverseOrder(QList<PageTreeItem*> items) {
  for (auto i = items.crbegin(); i != items.crend(); i++) {
    CloseItem(*i);
  }
}

void PageTree::DuplicateTree(PageTreeItem* item, PageTreeItem* to_parent) {
  // No parent means grab from item (which can still be no parent)
  if (!to_parent) to_parent = item->Parent();
  // Duplicate myself first, then children
  auto new_item = NewPage(item->Browser()->CurrentUrl(), to_parent, false);
  for (int i = 0; i < item->childCount(); i++) {
    DuplicateTree(AsPageTreeItem(item->child(i)), new_item);
  }
  new_item->setExpanded(item->isExpanded());
}

QList<PageTreeItem*> PageTree::Items() {
  QList<PageTreeItem*> ret;
  QTreeWidgetItemIterator it(this);
  while (*it) {
    auto item = AsPageTreeItem(*it);
    if (item) ret.append(item);
    it++;
  }
  return ret;
}

QList<PageTreeItem*> PageTree::SelectedItems() {
  QList<PageTreeItem*> ret;
  QTreeWidgetItemIterator it(this);
  while (*it) {
    if ((*it)->isSelected()) {
      auto item = AsPageTreeItem(*it);
      ret.append(item);
    }
    it++;
  }
  return ret;
}

QList<PageTreeItem*> PageTree::SelectedItemsOnlyHighestLevel() {
  QList<PageTreeItem*> ret;
  for (int i = 0; i < topLevelItemCount(); i++) {
    auto item = AsPageTreeItem(topLevelItem(i));
    if (item) {
      ret.append(item->SelfSelectedOrChildrenSelected());
    }
  }
  return ret;
}

QList<PageTreeItem*> PageTree::SameHostPages(PageTreeItem* to_comp) {
  auto host = QUrl(to_comp->Browser()->CurrentUrl()).host();
  QList<PageTreeItem*> ret;
  QTreeWidgetItemIterator it(this);
  while (*it) {
    auto item = AsPageTreeItem(*it);
    if (item && host == QUrl(item->Browser()->CurrentUrl()).host()) {
      ret.append(ret);
    }
    it++;
  }
  return ret;
}

}  // namespace doogie
