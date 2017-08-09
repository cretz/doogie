#include "page_tree.h"

#include <algorithm>

#include "action_manager.h"
#include "bubble_settings_dialog.h"
#include "profile.h"
#include "workspace_dialog.h"
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
          [=](const QModelIndex&, int, int) {
    if (topLevelItemCount() == 0) emit TreeEmpty();
  });

  // Each time one is selected, we need to make sure to show that on the stack
  connect(this, &QTreeWidget::currentItemChanged,
          [=](QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    // Deactivate previous
    if (previous && previous->type() == kPageItemType) {
      auto old_font = previous->font(0);
      old_font.setBold(false);
      previous->setFont(0, old_font);
    }
    // Before anything, if current is a workspace item, we don't support
    //  "current" so put it back
    if (current && current->type() == kWorkspaceItemType) {
      if (previous && previous->type() == kPageItemType) {
        setCurrentItem(previous);
      } else {
        setCurrentItem(nullptr);
      }
      return;
    }
    // Sometimes current isn't set or isn't in the tree anymore
    if (current && indexFromItem(current).isValid()) {
      auto page_item = AsPageTreeItem(current);
      browser_stack_->setCurrentWidget(page_item->Browser());
      auto new_font = page_item->font(0);
      new_font.setBold(true);
      page_item->setFont(0, new_font);
    } else {
      // As a special case, we need to set something as current
      // so we do it here. We try previous, but otherwise, we'll just
      // do the first one we can.
      if (previous && previous->type() != kWorkspaceItemType &&
          indexFromItem(previous).isValid()) {
        setCurrentItem(previous);
        return;
      }
      // Yeah this is a bit heavier than needed, but whatever
      for (auto item : Items()) {
        setCurrentItem(item);
        return;
      }
      // Oh well, no items, means they are all closed
    }
  });

  // Need to change workspace name if it changed
  connect(this, &PageTree::itemChanged,
          [=](QTreeWidgetItem* item, int column) {
    if (column == 0 && item->type() == kWorkspaceItemType) {
      AsWorkspaceTreeItem(item)->TextChanged();
    }
  });

  // Connect the simple close
  connect(this, &PageTree::ItemClose, [=](PageTreeItem* item) {
    CloseItem(item);
  });

  connect(this, &PageTree::ItemCloseRelease, [=](PageTreeItem* item) {
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

  connect(this, &PageTree::WorkspaceClosed, [=](qlonglong id) {
    // Remove the workspace from the open list
    auto list = Profile::Current()->OpenWorkspaceIds();
    list.removeAll(id);
    Profile::Current()->SetOpenWorkspaceIds(list);
    Profile::Current()->SavePrefs();
    // Make the current one implicit if necessary
    MakeWorkspaceImplicitIfPossible();
  });

  connect(this, &PageTree::itemCollapsed, [=](QTreeWidgetItem* item) {
    auto page_item = AsPageTreeItem(item);
    if (page_item) page_item->CollapseStateChanged();
  });
  connect(this, &PageTree::itemExpanded, [=](QTreeWidgetItem* item) {
    auto page_item = AsPageTreeItem(item);
    if (page_item) page_item->CollapseStateChanged();
  });

  SetupInitialWorkspaces();
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

PageTreeItem* PageTree::CurrentItem() const {
  return AsPageTreeItem(currentItem());
}

PageTreeItem* PageTree::NewPage(const QString &url,
                                PageTreeItem* parent,
                                bool make_current) {
  Workspace::WorkspacePage page;
  if (parent) {
    page.SetWorkspaceId(parent->WorkspacePage().WorkspaceId());
    page.SetParentId(parent->WorkspacePage().Id());
    page.SetBubble(parent->Browser()->CurrentBubble().Name());
  } else {
    page.SetWorkspaceId(WorkspaceToAddUnder().Id());
    page.SetBubble(Profile::Current()->DefaultBubble().Name());
  }
  page.SetUrl(url);
  return NewPage(page, parent, make_current);
}

PageTreeItem* PageTree::NewPage(Workspace::WorkspacePage& page,
                                PageTreeItem* parent,
                                bool make_current) {
  auto bubble_index = Profile::Current()->BubbleIndexFromName(page.Bubble());
  auto start_url = page.Url();
  if (bubble_index == -1) {
    qCritical() << "Workspace page used bubble '" << page.Bubble() <<
                   "' which doesn't exist anymore,"
                   " using default and suspending";
    page.SetBubble(Profile::Current()->DefaultBubble().Name());
    bubble_index = Profile::Current()->BubbleIndexFromName(page.Bubble());
    page.SetSuspended(true);
  }
  if (page.Suspended()) start_url = "";
  auto bubble = Profile::Current()->Bubbles()[bubble_index];
  auto browser = browser_stack_->NewBrowser(bubble, start_url);
  if (page.Suspended()) browser->SetSuspended(true);
  return AddBrowser(browser, page, parent, make_current);
}

void PageTree::ApplyBubbleSelectMenu(QMenu* menu,
                                     QList<PageTreeItem*> apply_to_items) {
  // If there is a commonly selected one for all of them, we will mark
  // that one as checked and unable to be selected
  QString common_bubble_name;
  for (auto item : apply_to_items) {
    auto bubble_name = item->Browser()->CurrentBubble().Name();
    if (common_bubble_name.isNull()) {
      common_bubble_name = bubble_name;
    } else if (common_bubble_name != bubble_name) {
      common_bubble_name = QString();
      break;
    }
  }
  for (auto& bubble : Profile::Current()->Bubbles()) {
    auto action = menu->addAction(bubble.Icon(), bubble.FriendlyName());
    if (!common_bubble_name.isNull() && common_bubble_name == bubble.Name()) {
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
    auto bubble_name = BubbleSettingsDialog::NewBubble();
    if (!bubble_name.isNull()) {
      auto bubble = Profile::Current()->
          Bubbles()[Profile::Current()->BubbleIndexFromName(bubble_name)];
      for (auto item : apply_to_items) {
        item->SetCurrentBubbleIfDifferent(bubble);
      }
    }
  });
}

void PageTree::ApplyRecentWorkspacesMenu(QMenu* menu) {
  QSet<qlonglong> exclude;
  for (auto& workspace : Workspaces()) exclude.insert(workspace.Id());
  for (auto& workspace : Workspace::RecentWorkspaces(exclude, 10)) {
    auto id = workspace.Id();
    menu->addAction(workspace.FriendlyName(), [=]() {
      OpenWorkspace(Workspace(id));
    });
  }
}

void PageTree::ApplyWorkspaceMenu(QMenu* menu,
                                  const Workspace& workspace,
                                  WorkspaceTreeItem* item) {
  menu->addAction("Change name", [=]() {
    if (item) {
      editItem(item);
    } else {
      bool failed_try_again;
      do {
        failed_try_again = false;
        auto new_name = QInputDialog::getText(
              nullptr, "New Workspace Name",
              "Workspace Name:", QLineEdit::Normal,
              workspace.Name());
        if (!new_name.isNull() && new_name != workspace.Name()) {
          if (Workspace::NameInUse(new_name)) {
            QMessageBox::critical(nullptr,
                                  "Invalid Name",
                                  "Name already in use by another workspace");
            failed_try_again = true;
          } else {
            implicit_workspace_.SetName(new_name);
            implicit_workspace_.Save();
            emit WorkspaceImplicitnessChanged();
          }
        }
      } while (failed_try_again);
    }
  });
  auto ids = Profile::Current()->OpenWorkspaceIds();
  auto curr_index = ids.indexOf(workspace.Id());
  menu->addAction("Move Up", [=]() {
    auto item = takeTopLevelItem(curr_index);
    auto expanded = item->isExpanded();
    insertTopLevelItem(curr_index - 1, item);
    item->setExpanded(expanded);
    auto ids = Profile::Current()->OpenWorkspaceIds();
    ids.removeAll(workspace.Id());
    ids.insert(curr_index - 1, workspace.Id());
    Profile::Current()->SetOpenWorkspaceIds(ids);
    Profile::Current()->SavePrefs();
  })->setEnabled(item && curr_index > 0);
  menu->addAction("Move Down", [=]() {
    auto item = takeTopLevelItem(curr_index);
    auto expanded = item->isExpanded();
    insertTopLevelItem(curr_index + 1, item);
    item->setExpanded(expanded);
    auto ids = Profile::Current()->OpenWorkspaceIds();
    ids.removeAll(workspace.Id());
    ids.insert(curr_index + 1, workspace.Id());
    Profile::Current()->SetOpenWorkspaceIds(ids);
    Profile::Current()->SavePrefs();
  })->setEnabled(item && curr_index < ids.length() - 1);
  menu->addSeparator();
  menu->addAction("Close", [=]() {
    CloseWorkspace(item);
  })->setEnabled(item);
  menu->addAction("Close and Delete", [=]() {
    auto res = QMessageBox::question(
          nullptr, "Delete Workspace?",
          QString("Are you sure you want to delete workspace '%1'").arg(
            workspace.FriendlyName()));
    if (res == QMessageBox::Yes) {
      item->SetDeleteWorkspaceOnDestroyNotCancelled(true);
      CloseWorkspace(item);
    }
  })->setEnabled(item);
  menu->addSeparator();
  menu->addAction("Close Other Workspaces", [=]() {
    // Collect all top level items that are not this one
    QList<WorkspaceTreeItem*> other;
    for (int i = 0; i < topLevelItemCount(); i++) {
      auto item = AsWorkspaceTreeItem(topLevelItem(i));
      if (item->CurrentWorkspace().Id() != workspace.Id()) {
        other.append(item);
      }
    }
    for (auto item : other) {
      CloseWorkspace(item);
    }
  })->setEnabled(item);
}

WorkspaceTreeItem* PageTree::OpenWorkspace(Workspace& workspace) {
  MakeWorkspaceExplicitIfPossible();

  // Mark as opened
  workspace.SetLastOpened(QDateTime::currentMSecsSinceEpoch());
  workspace.Save();
  auto item = new WorkspaceTreeItem(workspace);
  addTopLevelItem(item);

  // Save that it's open in the profile
  auto opened_list = Profile::Current()->OpenWorkspaceIds();
  opened_list.removeAll(workspace.Id());
  opened_list.append(workspace.Id());
  Profile::Current()->SetOpenWorkspaceIds(opened_list);
  Profile::Current()->SavePrefs();

  // Let's add the children
  QHash<qlonglong, PageTreeItem*> items_by_page_id;
  QHash<qlonglong, QList<Workspace::WorkspacePage>> children_by_parent_id;
  for (auto& child : workspace.AllChildren()) {
    children_by_parent_id[child.ParentId()].append(child);
  }
  std::function<void(qlonglong)> add_children_of =
      [=, &items_by_page_id, &add_children_of](qlonglong id) {
    auto parent = items_by_page_id.value(id);
    for (auto child : children_by_parent_id[id]) {
      auto expanded = child.Expanded();
      items_by_page_id[child.Id()] = NewPage(child, parent, false);
      add_children_of(child.Id());
      items_by_page_id[child.Id()]->setExpanded(expanded);
    }
  };
  add_children_of(-1);
  // Expand by default
  item->setExpanded(true);

  // If there is nothing current, set it as  child of this or just any
  // we can find
  if (!currentItem()) {
    if (item->childCount() > 0) {
      setCurrentItem(item->child(0));
    } else {
      // This is heavy, but who cares
      for (auto item : Items()) {
        setCurrentItem(item);
        break;
      }
    }
  }

  return item;
}

void PageTree::CloseAllWorkspaces() {
  if (has_implicit_workspace_) {
    // Just close all pages
    CloseItemsInReverseOrder(Items(), false);
    return;
  }
  // Close the workspaces in reverse
  for (int i = topLevelItemCount() - 1; i >= 0; i--) {
    // Don't send event
    CloseWorkspace(AsWorkspaceTreeItem(topLevelItem(i)), false);
  }
  // If the top one was made implicit in the meantime,
  // we have to try and close the items again
  if (has_implicit_workspace_) CloseItemsInReverseOrder(Items(), false);
}

void PageTree::WorkspaceAboutToDestroy(WorkspaceTreeItem* item) {
  // If current item is empty or no longer valid, we try to set the
  // current item as the one above us
  if (!currentItem() || !indexFromItem(currentItem()).isValid() ||
      AsPageTreeItem(currentItem())->CurrentWorkspace().Id() ==
            item->CurrentWorkspace().Id()) {
    auto new_curr_item = itemAbove(item);
    if (!new_curr_item) new_curr_item = itemBelow(item);
    setCurrentItem(new_curr_item);
  }
}

QJsonObject PageTree::DebugDump() const {
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
      CloseItem(affected, true, true);
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

  auto wks_menu = menu.addMenu("Workspaces");
  wks_menu->addAction(ActionManager::Action(
      ActionManager::NewWorkspace));
  wks_menu->addAction(ActionManager::Action(
      ActionManager::OpenWorkspace));
  auto recent_wks_menu = wks_menu->addMenu("Open Recent Workspace");
  connect(recent_wks_menu, &QMenu::aboutToShow, [=]() {
    if (recent_wks_menu->isEmpty()) {
      ApplyRecentWorkspacesMenu(recent_wks_menu);
      if (recent_wks_menu->isEmpty()) {
        recent_wks_menu->addAction("<none>")->setEnabled(false);
      }
    }
  });
  wks_menu->addAction(ActionManager::Action(
      ActionManager::ManageWorkspaces));

  // The workspace will be the implicit one or the one the pointer is in
  Workspace workspace = implicit_workspace_;
  WorkspaceTreeItem* wk_item = nullptr;
  if (!has_implicit_workspace_) {
    for (int i = 0; i < topLevelItemCount(); i++) {
      auto item = topLevelItem(i);
      auto rect = visualItemRect(item);
      if (rect.top() > event->y()) break;
      wk_item = AsWorkspaceTreeItem(item);
      workspace = wk_item->CurrentWorkspace();
    }
  }

  auto wk_menu = menu.addMenu(
      QString("Workspace: ") + workspace.FriendlyName());
  ApplyWorkspaceMenu(wk_menu, workspace, wk_item);

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
  if (event->source() == this) {
    // Due to bad internal Qt logic, we reset the current here
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
    for (auto url : data->urls()) {
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
  // As a special case, if we do not have an implicit workspace and rows are
  // inserted at the top level, we want the workspace above them to add to
  if (!has_implicit_workspace_ && !parent.isValid() &&
      topLevelItem(start)->type() == kPageItemType) {
    // Take all of the items and re-insert them under their workspace item
    auto workspace_item = AsWorkspaceTreeItem(topLevelItem(start - 1));
    if (!workspace_item) workspace_item = WorkspaceTreeItemToAddUnder();
    for (int i = start; i <= end; i++) {
      workspace_item->addChild(takeTopLevelItem(start));
    }
    return;
  }
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
          &QAction::triggered, [=]() {
    auto page = NewPage("", nullptr, true);
    // Focus URL edit
    page->Browser()->FocusUrlEdit();
  });
  connect(ActionManager::Action(ActionManager::NewChildForegroundPage),
          &QAction::triggered, [=]() {
    auto page = NewPage("", CurrentItem(), true);
    // Focus URL edit
    page->Browser()->FocusUrlEdit();
  });
  connect(ActionManager::Action(ActionManager::NewChildBackgroundPage),
          &QAction::triggered,
          [=]() { NewPage("", CurrentItem(), false); });
  connect(ActionManager::Action(ActionManager::Reload),
          &QAction::triggered,
          [=]() { CurrentItem()->Browser()->Refresh(); });
  connect(ActionManager::Action(ActionManager::SuspendPage),
          &QAction::triggered,
          [=]() { CurrentItem()->Browser()->SetSuspended(true); });
  connect(ActionManager::Action(ActionManager::UnsuspendPage),
          &QAction::triggered,
          [=]() { CurrentItem()->Browser()->SetSuspended(false); });
  connect(ActionManager::Action(ActionManager::Stop),
          &QAction::triggered,
          [=]() { CurrentItem()->Browser()->Stop(); });
  connect(ActionManager::Action(ActionManager::Forward),
          &QAction::triggered,
          [=]() { CurrentItem()->Browser()->Forward(); });
  connect(ActionManager::Action(ActionManager::Print),
          &QAction::triggered,
          [=]() { CurrentItem()->Browser()->Print(); });
  connect(ActionManager::Action(ActionManager::ZoomIn),
          &QAction::triggered, [=]() {
    CurrentItem()->Browser()->SetZoomLevel(
          CurrentItem()->Browser()->ZoomLevel() + 0.1);
  });
  connect(ActionManager::Action(ActionManager::ZoomOut),
          &QAction::triggered, [=]() {
    CurrentItem()->Browser()->SetZoomLevel(
          CurrentItem()->Browser()->ZoomLevel() - 0.1);
  });
  connect(ActionManager::Action(ActionManager::ResetZoom),
          &QAction::triggered,
          [=]() { CurrentItem()->Browser()->SetZoomLevel(0.0); });
  connect(ActionManager::Action(ActionManager::FindInPage),
          &QAction::triggered,
          [=]() { CurrentItem()->Browser()->ShowFind(); });
  connect(ActionManager::Action(ActionManager::ExpandTree),
          &QAction::triggered,
          [=]() { CurrentItem()->ExpandSelfAndChildren(); });
  connect(ActionManager::Action(ActionManager::CollapseTree),
          &QAction::triggered,
          [=]() { CurrentItem()->CollapseSelfAndChildren(); });
  connect(ActionManager::Action(ActionManager::DuplicateTree),
          &QAction::triggered,
          [=]() { DuplicateTree(CurrentItem()); });
  connect(ActionManager::Action(ActionManager::SelectSameHostPages),
          &QAction::triggered, [=]() {
    clearSelection();
    for (auto item : SameHostPages(CurrentItem())) {
      item->setSelected(true);
    }
  });
  connect(ActionManager::Action(ActionManager::ClosePage),
          &QAction::triggered, [=]() {
    // Expand it to prevent child close
    CurrentItem()->setExpanded(true);
    CloseItem(CurrentItem());
  });
  connect(ActionManager::Action(ActionManager::CloseTree),
          &QAction::triggered, [=]() {
    CloseItem(CurrentItem(), true, true);
  });
  connect(ActionManager::Action(ActionManager::CloseSameHostPages),
          &QAction::triggered, [=]() {
    CloseItemsInReverseOrder(SameHostPages(CurrentItem()));
  });
  connect(ActionManager::Action(ActionManager::CloseOtherTrees),
          &QAction::triggered,
          [=]() { CloseItemsInReverseOrder(CurrentItem()->Siblings()); });

  connect(ActionManager::Action(ActionManager::ReloadSelectedPages),
          &QAction::triggered, [=]() {
    for (auto item : SelectedItems()) {
      item->Browser()->Refresh();
    }
  });
  connect(ActionManager::Action(ActionManager::SuspendSelectedPages),
          &QAction::triggered, [=]() {
    for (auto item : SelectedItems()) {
      item->Browser()->SetSuspended(true);
    }
  });
  connect(ActionManager::Action(ActionManager::UnsuspendSelectedPages),
          &QAction::triggered, [=]() {
    for (auto item : SelectedItems()) {
      item->Browser()->SetSuspended(false);
    }
  });
  connect(ActionManager::Action(ActionManager::ExpandSelectedTrees),
          &QAction::triggered, [=]() {
    for (auto item : SelectedItemsOnlyHighestLevel()) {
      item->ExpandSelfAndChildren();
    }
  });
  connect(ActionManager::Action(ActionManager::CollapseSelectedTrees),
          &QAction::triggered, [=]() {
    for (auto item : SelectedItemsOnlyHighestLevel()) {
      item->CollapseSelfAndChildren();
    }
  });
  // We accept that this ignores child selection if an ancestor
  // is selected. Otherwise, the definition of child+parent duplication
  // can be ambiguous depending on which you duplicate first.
  connect(ActionManager::Action(ActionManager::DuplicateSelectedTrees),
          &QAction::triggered, [=]() {
    for (auto item : SelectedItemsOnlyHighestLevel()) {
      DuplicateTree(item);
    }
  });
  connect(ActionManager::Action(ActionManager::CloseSelectedPages),
          &QAction::triggered, [=]() {
    // Go backwards and expand before closing
    auto items = SelectedItems();
    for (auto i = items.crbegin(); i != items.crend(); i++) {
      (*i)->setExpanded(true);
      CloseItem(*i);
    }
  });
  connect(ActionManager::Action(ActionManager::CloseSelectedTrees),
          &QAction::triggered, [=]() {
    for (auto item : SelectedItemsOnlyHighestLevel()) {
      CloseItem(item, true, true);
    }
  });
  connect(ActionManager::Action(ActionManager::CloseNonSelectedPages),
          &QAction::triggered, [=]() {
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
          &QAction::triggered, [=]() {
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
          &QAction::triggered, [=]() {
    for (auto item : Items()) {
      item->Browser()->Refresh();
    }
  });
  connect(ActionManager::Action(ActionManager::SuspendAllPages),
          &QAction::triggered, [=]() {
    for (auto item : Items()) {
      item->Browser()->SetSuspended(true);
    }
  });
  connect(ActionManager::Action(ActionManager::UnsuspendAllPages),
          &QAction::triggered, [=]() {
    for (auto item : Items()) {
      item->Browser()->SetSuspended(false);
    }
  });
  connect(ActionManager::Action(ActionManager::ExpandAllTrees),
          &QAction::triggered, [=]() {
    for (auto item : Items()) {
      item->setExpanded(true);
    }
  });
  connect(ActionManager::Action(ActionManager::CollapseAllTrees),
          &QAction::triggered, [=]() {
    for (auto item : Items()) {
      item->setExpanded(false);
    }
  });
  connect(ActionManager::Action(ActionManager::CloseAllPages),
          &QAction::triggered,
          [=]() { CloseItemsInReverseOrder(Items()); });
  connect(ActionManager::Action(ActionManager::FocusPageTree),
          &QAction::triggered,
          [=]() { setFocus(); });

  connect(ActionManager::Action(ActionManager::NewWorkspace),
          &QAction::triggered, [=]() {
    // Create the workspace w/ the next name
    Workspace workspace;
    workspace.SetName(Workspace::NextUnusedWorkspaceName());
    workspace.Save();
    auto item = OpenWorkspace(workspace);
    editItem(item);
  });
  connect(ActionManager::Action(ActionManager::ManageWorkspaces),
          &QAction::triggered, [=]() {
    WorkspaceDialog(window()).execManage(Workspaces());
  });
  connect(ActionManager::Action(ActionManager::OpenWorkspace),
          &QAction::triggered, [=]() {
    WorkspaceDialog dialog(window());
    if (dialog.execOpen(Workspaces()) == QDialog::Accepted) {
      OpenWorkspace(dialog.SelectedWorkspace());
    }
  });
}

void PageTree::SetupInitialWorkspaces() {
  auto found_any = false;
  for (auto id : Profile::Current()->OpenWorkspaceIds()) {
    Workspace workspace(id);
    if (workspace.Exists()) {
      found_any = true;
      qDebug() << "Opening previous workspace ID " << id;
      OpenWorkspace(workspace);
    }
  }
  if (!found_any) {
    // Just try the most recent
    auto recent = Workspace::RecentWorkspaces({}, 1);
    if (!recent.isEmpty()) {
      qDebug() << "Using most recent workspace";
      OpenWorkspace(recent.first());
    } else {
      // Just create a default one
      Workspace workspace;
      qDebug() << "Opening default workspace";
      OpenWorkspace(workspace);
    }
  }
  MakeWorkspaceImplicitIfPossible();
}

PageTreeItem* PageTree::AddBrowser(QPointer<BrowserWidget> browser,
                                   Workspace::WorkspacePage& page,
                                   PageTreeItem* parent,
                                   bool make_current) {
  // Create the tree item
  page.SetPos(parent ? parent->childCount() : topLevelItemCount());
  page.Save();
  auto browser_item = new PageTreeItem(browser, page);

  // Put as top level or as child
  if (parent) {
    parent->addChild(browser_item);
    // If the child is the first child, we expand automatically
    if (parent->childCount() == 1) {
      parent->setExpanded(true);
    }
  } else {
    auto workspace_to_add_under = WorkspaceTreeItemToAddUnder();
    if (workspace_to_add_under) {
      workspace_to_add_under->addChild(browser_item);
    } else {
      addTopLevelItem(browser_item);
    }
  }
  // Whether to set it as the current active
  if (make_current) {
    setCurrentItem(browser_item, 0, QItemSelectionModel::Current);
  }
  browser_item->setExpanded(page.Expanded());
  // Make all tab opens open as child
  connect(browser, &BrowserWidget::PageOpen,
          [=](CefHandler::WindowOpenType type,
              const QString& url,
              bool user_gesture) {
    PageTreeItem* parent = nullptr;
    // TODO(cretz): better UI and should I block all non-user page changes?
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

void PageTree::CloseWorkspace(WorkspaceTreeItem* item, bool send_close_event) {
  // Do nothing if no item
  if (!item) return;
  // If it's empty, just destroy it
  if (item->childCount() == 0) {
    auto id = item->CurrentWorkspace().Id();
    WorkspaceAboutToDestroy(item);
    delete item;
    if (send_close_event) emit WorkspaceClosed(id);
    return;
  }
  // Otherwise, try to collapse and close all children in reverse
  item->SetCloseOnEmptyNotCancelled(true);
  item->SetSendCloseEventNotCancelled(send_close_event);
  for (int i = item->childCount() - 1; i >= 0; i--) {
    auto child = AsPageTreeItem(item->child(i));
    CloseItem(child, false, true);
  }
}

void PageTree::CloseItem(PageTreeItem* item,
                         bool workspace_persist,
                         bool force_close_children) {
  // Eagerly skip this if the item ain't a thing anymore
  if (!item) return;
  item->SetPersistNextCloseToWorkspace(workspace_persist);
  // We only close children if we're not expanded unless we're foreced
  if (force_close_children || !item->isExpanded()) {
    // Close backwards up the list
    for (int i = item->childCount() - 1; i >= 0; i--) {
      auto child = AsPageTreeItem(item->child(i));
      child->SetPersistNextCloseToWorkspace(workspace_persist);
      CloseItem(child, workspace_persist, force_close_children);
    }
  }
  // Now we can close myself
  item->Browser()->TryClose();
}

void PageTree::CloseItemsInReverseOrder(QList<PageTreeItem*> items,
                                        bool workspace_persist) {
  for (auto i = items.crbegin(); i != items.crend(); i++) {
    CloseItem(*i, workspace_persist);
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

QList<Workspace> PageTree::Workspaces() const {
  if (has_implicit_workspace_) return { implicit_workspace_ };
  QList<Workspace> ret;
  for (int i = 0; i < topLevelItemCount(); i++) {
    auto item = AsWorkspaceTreeItem(topLevelItem(i));
    if (item) ret.append(item->CurrentWorkspace());
  }
  return ret;
}

const Workspace& PageTree::WorkspaceToAddUnder() const {
  if (has_implicit_workspace_) return implicit_workspace_;
  return AsWorkspaceTreeItem(
        topLevelItem(topLevelItemCount() - 1))->CurrentWorkspace();
}

WorkspaceTreeItem* PageTree::WorkspaceTreeItemToAddUnder() const {
  if (has_implicit_workspace_) return nullptr;
  return AsWorkspaceTreeItem(topLevelItem(topLevelItemCount() - 1));
}

void PageTree::MakeWorkspaceExplicitIfPossible() {
  if (!has_implicit_workspace_) return;
  auto current = currentItem();
  has_implicit_workspace_ = false;
  auto item = new WorkspaceTreeItem(implicit_workspace_);
  insertTopLevelItem(0, item);
  while (auto child = AsPageTreeItem(takeTopLevelItem(1))) {
    item->addChild(child);
    child->AfterAdded();
  }
  // This needs to be expanded
  item->setExpanded(true);
  // Reset the current item
  setCurrentItem(current);
  emit WorkspaceImplicitnessChanged();
}

void PageTree::MakeWorkspaceImplicitIfPossible() {
  if (has_implicit_workspace_ || topLevelItemCount() != 1) return;
  auto current = currentItem();
  has_implicit_workspace_ = true;
  auto item = AsWorkspaceTreeItem(takeTopLevelItem(0));
  implicit_workspace_ = item->CurrentWorkspace();
  while (auto child = AsPageTreeItem(item->takeChild(0))) {
    addTopLevelItem(child);
    // We have to re-set the expansion here
    child->setExpanded(child->WorkspacePage().Expanded());
    child->AfterAdded();
  }
  // Reset the current item
  setCurrentItem(current);
  emit WorkspaceImplicitnessChanged();
}

}  // namespace doogie
