#include "page_tree.h"
#include <algorithm>
#include "action_manager.h"

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

  // Emit empty on row removal
  connect(model(), &QAbstractItemModel::rowsRemoved,
          [this](const QModelIndex&, int, int) {
    if (topLevelItemCount() == 0) emit TreeEmpty();
  });

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
  return static_cast<PageTreeItem*>(currentItem());
}

PageTreeItem* PageTree::NewPage(const QString &url,
                                PageTreeItem* parent,
                                bool make_current) {
  auto browser = browser_stack_->NewBrowser(url);
  return AddBrowser(browser, parent, make_current);
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

void PageTree::contextMenuEvent(QContextMenuEvent* event) {
  QMenu menu(this);

  menu.addAction(ActionManager::Action(ActionManager::NewTopLevelPage));

  // Single-page
  auto clicked_on = static_cast<PageTreeItem*>(itemAt(event->pos()));
  auto current = static_cast<PageTreeItem*>(currentItem());
  auto affected = (clicked_on) ? clicked_on : current;
  if (affected) {
    QString type = (clicked_on) ? "Clicked-On" : "Current";
    menu.addSection(type + " Page");
    menu.addAction("New Child Background Page", [this, affected]() {
      NewPage("", affected, false);
    });
    menu.addAction("New Foreground Child Page", [this, affected]() {
      NewPage("", affected, false);
    });
    menu.addAction("Reload " + type + " Page", [this, affected]() {
      affected->Browser()->Refresh();
    });
    menu.addAction("Expand " + type + " Tree", [this, affected]() {
      affected->ExpandSelfAndChildren();
    })->setEnabled(affected->SelfOrAnyChildCollapsed());
    menu.addAction("Collapse " + type + " Tree", [this, affected]() {
      affected->CollapseSelfAndChildren();
    })->setEnabled(affected->SelfOrAnyChildExpanded());
    menu.addAction("Duplicate " + type + " Tree", [this, affected]() {
      DuplicateTree(affected);
    });
    menu.addAction("Select " + type + " Same-Host Pages", [this, affected]() {
      clearSelection();
      for (const auto& item : SameHostPages(affected)) {
        item->setSelected(true);
      }
    });
    menu.addAction("Close " + type + " Page", [this, affected]() {
      // We expand this to force children upwards
      affected->setExpanded(true);
      CloseItem(affected);
    });
    menu.addAction("Close " + type + " Tree", [this, affected]() {
      // We collapse everything here to force children to close
      affected->CollapseSelfAndChildren();
      CloseItem(affected);
    });
    menu.addAction("Close " + type + " Same-Host Pages", [this, affected]() {
      CloseItemsInReverseOrder(SameHostPages(affected));
    });
    menu.addAction("Close Non-" + type + " Trees", [this, affected]() {
      CloseItemsInReverseOrder(affected->Siblings());
    });
  }

  auto selected_indices = selectedIndexes();
  auto has_selected_not_affected = selected_indices.count() > 1 ||
      (selected_indices.count() == 1 &&
        (!affected || selected_indices[0] != indexFromItem(affected)));
  if (has_selected_not_affected) {
    menu.addSection("Selected Pages");
    menu.addAction(ActionManager::Action(
        ActionManager::ReloadSelectedPages));
    menu.addAction(ActionManager::Action(
        ActionManager::ExpandSelectedTrees));
    menu.addAction(ActionManager::Action(
        ActionManager::CollapseSelectedTrees));
    menu.addAction(ActionManager::Action(
        ActionManager::DuplicateSelectedTrees));
    menu.addAction(ActionManager::Action(
        ActionManager::CloseSelectedPages));
    menu.addAction(ActionManager::Action(
        ActionManager::CloseSelectedTrees));
    menu.addAction(ActionManager::Action(
        ActionManager::CloseNonSelectedPages));
    menu.addAction(ActionManager::Action(
        ActionManager::CloseNonSelectedTrees));
  }

  if (topLevelItemCount() > 0) {
    menu.addSection("All Pages");
    menu.addAction(ActionManager::Action(
        ActionManager::ReloadAllPages));
    menu.addAction(ActionManager::Action(
        ActionManager::ExpandAllTrees));
    menu.addAction(ActionManager::Action(
        ActionManager::CollapseAllTrees));
    menu.addAction(ActionManager::Action(
        ActionManager::CloseAllPages));
  }

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
    auto browser = static_cast<PageTreeItem*>(item)->Browser();
    if (browser) {
      auto url = browser->CurrentUrl();
      if (!url.isEmpty()) urls.append(QUrl(url));
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
    NewPage("", parent, true);
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
    auto item = static_cast<PageTreeItem*>(
          itemFromIndex(model()->index(i, 0, parent)));
    item->AfterAdded();
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
    AddBrowser(browser_stack_->NewBrowser(url), parent, make_current);
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
      CloseItem(static_cast<PageTreeItem*>(item->child(i)));
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
  auto new_item = AddBrowser(
        browser_stack_->NewBrowser(item->Browser()->CurrentUrl()),
        to_parent, false);
  for (int i = 0; i < item->childCount(); i++) {
    DuplicateTree(static_cast<PageTreeItem*>(item->child(i)), new_item);
  }
  new_item->setExpanded(item->isExpanded());
}

QList<PageTreeItem*> PageTree::Items() {
  QList<PageTreeItem*> ret;
  QTreeWidgetItemIterator it(this);
  while (*it) {
    ret.append(static_cast<PageTreeItem*>(*it));
    it++;
  }
  return ret;
}

QList<PageTreeItem*> PageTree::SelectedItems() {
  QList<PageTreeItem*> ret;
  QTreeWidgetItemIterator it(this);
  while (*it) {
    if ((*it)->isSelected()) {
      ret.append(static_cast<PageTreeItem*>(*it));
    }
    it++;
  }
  return ret;
}

QList<PageTreeItem*> PageTree::SelectedItemsOnlyHighestLevel() {
  QList<PageTreeItem*> ret;
  for (int i = 0; i < topLevelItemCount(); i++) {
    ret.append(static_cast<PageTreeItem*>(topLevelItem(i))->
               SelfSelectedOrChildrenSelected());
  }
  return ret;
}

QList<PageTreeItem*> PageTree::SameHostPages(PageTreeItem* to_comp) {
  auto host = QUrl(to_comp->Browser()->CurrentUrl()).host();
  QList<PageTreeItem*> ret;
  QTreeWidgetItemIterator it(this);
  while (*it) {
    auto item = static_cast<PageTreeItem*>(*it);
    if (host == QUrl(item->Browser()->CurrentUrl()).host()) {
      ret.append(ret);
    }
    it++;
  }
  return ret;
}

}  // namespace doogie
