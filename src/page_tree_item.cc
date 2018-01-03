#include "page_tree_item.h"

#include "page_tree.h"
#include "util.h"
#include "workspace_tree_item.h"

namespace doogie {

PageTreeItem::PageTreeItem(QPointer<BrowserWidget> browser,
                           const Workspace::WorkspacePage& workspace_page)
    : QTreeWidgetItem(PageTree::kPageItemType),
      browser_(browser),
      workspace_page_(workspace_page) {
  if (!workspace_page_.Exists()) workspace_page_.Persist();
  if (!workspace_page_.Icon().isNull()) {
    setIcon(0, workspace_page_.Icon());
    setText(0, workspace_page_.Title());
    browser->SetUrlText(workspace_page_.Url());
  } else {
    setText(0, "(New Window)");
  }

  setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
  // Connect title and favicon change
  browser->connect(browser, &BrowserWidget::TitleChanged, [=]() {
    if (browser_) {
      setText(0, browser_->CurrentTitle());
      setToolTip(0, browser_->CurrentTitle());
      workspace_page_.SetTitle(browser_->CurrentTitle());
      workspace_page_.Persist();
    }
  });
  browser->connect(browser, &BrowserWidget::LoadingStateChanged, [=]() {
    ApplyFavicon();
    if (browser_->CurrentUrl() != workspace_page_.Url()) {
      workspace_page_.SetUrl(browser_->CurrentUrl());
      workspace_page_.Persist();
    }
  });
  browser->connect(browser, &BrowserWidget::FaviconChanged, [=]() {
    ApplyFavicon();
    workspace_page_.SetIcon(browser_->CurrentFavicon());
    workspace_page_.Persist();
  });
  browser->connect(browser, &BrowserWidget::destroyed, [=]() {
    // Mark myself invalid so I don't accidentally set myself
    valid_ = false;
    // If I was current, set the new current as either the prev or next
    if (treeWidget() && (!treeWidget()->currentItem() ||
                         treeWidget()->currentItem() == this)) {
      static_cast<PageTree*>(treeWidget())->SetCurrentClosestTo(this);
    }
    // Move all the children up
    if (parent()) {
      parent()->insertChildren(parent()->indexOfChild(this), takeChildren());
    } else {
      treeWidget()->insertTopLevelItems(
            treeWidget()->indexOfTopLevelItem(this), takeChildren());
    }
    if (persist_next_close_to_workspace_) {
      workspace_page_.Delete();
    }
    auto workspace_item = WorkspaceItem();
    delete this;
    if (workspace_item) workspace_item->ChildCloseCompleted();
  });
  browser->connect(browser, &BrowserWidget::AboutToShowJSDialog,
                   [=]() {
    treeWidget()->setCurrentItem(this);
  });
  browser->connect(browser, &BrowserWidget::CloseCancelled, [=]() {
    persist_next_close_to_workspace_ = true;
    close_button_->setChecked(false);
    auto workspace_item = WorkspaceItem();
    if (workspace_item) workspace_item->ChildCloseCancelled();
  });
  browser->connect(browser, &BrowserWidget::SuspensionChanged, [=]() {
    auto palette = QGuiApplication::palette();
    if (browser_->Suspended()) {
      // If it's still loading, re-apply the favicon here so
      //  the loading movie stops
      if (browser_->Loading()) ApplyFavicon();
      setForeground(0, palette.brush(QPalette::Disabled, QPalette::Text));
      auto existing_icon = icon(0);
      auto sizes = existing_icon.availableSizes();
      if (!sizes.isEmpty()) {
        setIcon(0, QIcon(existing_icon.pixmap(sizes[0], QIcon::Disabled)));
      } else {
        setIcon(0, QIcon());
      }
    } else {
      setForeground(0, palette.brush(QPalette::Active, QPalette::Text));
      // Icon change should happen on load
    }
    workspace_page_.SetSuspended(browser_->Suspended());
    workspace_page_.Persist();
  });
  browser->connect(browser, &BrowserWidget::BubbleMaybeChanged, [=]() {
    auto label = qobject_cast<QLabel*>(
          treeWidget()->itemWidget(this, kBubbleIconColumn));
    if (label) {
      label->setPixmap(browser_->CurrentBubble().Icon().pixmap(16, 16));
      label->setToolTip("Bubble: " + browser_->CurrentBubble().FriendlyName());
    }
    workspace_page_.SetBubbleId(browser_->CurrentBubble().Id());
    workspace_page_.Persist();
  });
}

PageTreeItem::~PageTreeItem() {
  if (loading_icon_frame_conn_) {
    treeWidget()->disconnect(loading_icon_frame_conn_);
  }
}

QPointer<BrowserWidget> PageTreeItem::Browser() const {
  return valid_ ? browser_ : nullptr;
}

void PageTreeItem::AfterAdded() {
  // We need to update the workspace and parent if necessary
  auto workspace = CurrentWorkspace();
  auto par = Parent();
  if (workspace.Id() != workspace_page_.WorkspaceId() ||
      workspace_page_.ParentId() != (par ? par->WorkspacePage().Id() : -1)) {
    workspace_page_.SetWorkspaceId(workspace.Id());
    workspace_page_.SetParentId(par ? par->WorkspacePage().Id() : -1);
    workspace_page_.Persist();
  }

  auto tree = static_cast<PageTree*>(treeWidget());
  // Apply with the current icon if it's there
  ApplyFavicon(icon(0));

  // Set the icon
  auto label = new QLabel;
  label->setAlignment(Qt::AlignCenter);
  label->setPixmap(browser_->CurrentBubble().Icon().pixmap(16, 16));
  label->setToolTip("Bubble: " + browser_->CurrentBubble().FriendlyName());
  tree->setItemWidget(this, kBubbleIconColumn, label);
  label->setContextMenuPolicy(Qt::CustomContextMenu);
  tree->connect(label, &QLabel::customContextMenuRequested, [=]() {
    QMenu menu;
    tree->ApplyBubbleSelectMenu(&menu, { this });
    menu.exec(label->mapToGlobal(label->rect().bottomLeft()));
  });

  // Create a new close button everytime because it is destroyed otherwise
  close_button_ = new PageCloseButton(tree);
  tree->connect(close_button_, &PageCloseButton::Released,
                tree, &PageTree::ItemCloseRelease);
  tree->setItemWidget(this, kCloseButtonColumn, close_button_);
  // Gotta call it on my children too sadly
  for (int i = 0; i < childCount(); i++) {
    static_cast<PageTreeItem*>(child(i))->AfterAdded();
  }
}

PageTreeItem* PageTreeItem::Parent() const {
  if (parent() && parent()->type() == PageTree::kPageItemType) {
    return static_cast<PageTreeItem*>(parent());
  }
  return nullptr;
}

QJsonObject PageTreeItem::DebugDump() const {
  QJsonArray children;
  for (int i = 0; i < childCount(); i++) {
    children.append(static_cast<PageTreeItem*>(child(i))->DebugDump());
  }
  return {
    { "current", treeWidget()->currentItem() == this },
    { "expanded", isExpanded() },
    { "text", text(0) },
    { "rect", Util::DebugWidgetGeom(treeWidget(),
                                    treeWidget()->visualItemRect(this)) },
    { "browser", (browser_) ? browser_->DebugDump() : QJsonValue() },
    { "closeButton", QJsonObject({
      { "checked", close_button_->isChecked() },
      { "rect", Util::DebugWidgetGeom(close_button_) }
    })},
    { "items", children }
  };
}

QList<PageTreeItem*> PageTreeItem::SelfAndChildren() const {
  QList<PageTreeItem*> ret({ const_cast<PageTreeItem*>(this) });
  for (int i = 0; i < childCount(); i++) {
    ret << static_cast<PageTreeItem*>(child(i))->SelfAndChildren();
  }
  return ret;
}

bool PageTreeItem::SelfOrAnyChildCollapsed() const {
  if (childCount() > 0 && !isExpanded()) return true;
  for (int i = 0; i < childCount(); i++) {
    if (static_cast<PageTreeItem*>(child(i))->SelfOrAnyChildCollapsed()) {
      return true;
    }
  }
  return false;
}

bool PageTreeItem::SelfOrAnyChildExpanded() const {
  if (childCount() > 0 && isExpanded()) return true;
  for (int i = 0; i < childCount(); i++) {
    if (static_cast<PageTreeItem*>(child(i))->SelfOrAnyChildExpanded()) {
      return true;
    }
  }
  return false;
}

void PageTreeItem::ExpandSelfAndChildren() {
  setExpanded(true);
  for (int i = 0; i < childCount(); i++) {
    static_cast<PageTreeItem*>(child(i))->ExpandSelfAndChildren();
  }
}

void PageTreeItem::CollapseSelfAndChildren() {
  setExpanded(false);
  for (int i = 0; i < childCount(); i++) {
    static_cast<PageTreeItem*>(child(i))->CollapseSelfAndChildren();
  }
}

QList<PageTreeItem*> PageTreeItem::SelfSelectedOrChildrenSelected() const {
  if (isSelected()) return { const_cast<PageTreeItem*>(this) };
  QList<PageTreeItem*> ret;
  for (int i = 0; i < childCount(); i++) {
    ret.append(static_cast<PageTreeItem*>(child(i))->
        SelfSelectedOrChildrenSelected());
  }
  return ret;
}

bool PageTreeItem::SelectedOrHasSelectedParent() const {
  if (isSelected()) return true;
  if (!parent() || parent()->type() != PageTree::kPageItemType) return false;
  return static_cast<PageTreeItem*>(parent())->SelectedOrHasSelectedParent();
}

void PageTreeItem::ApplyWorkspaceExpansion() {
  setExpanded(workspace_page_.Expanded());
  for (int i = 0; i < childCount(); i++) {
    static_cast<PageTreeItem*>(child(i))->ApplyWorkspaceExpansion();
  }
}

QList<PageTreeItem*> PageTreeItem::Siblings() const {
  auto my_parent = parent();
  if (!my_parent) my_parent = treeWidget()->invisibleRootItem();
  QList<PageTreeItem*> ret;
  for (int i = 0; i < my_parent->childCount(); i++) {
    auto item = my_parent->child(i);
    if (item && item->type() == PageTree::kPageItemType) {
      ret.append(static_cast<PageTreeItem*>(item));
    }
  }
  return ret;
}

void PageTreeItem::SetCurrentBubbleIfDifferent(const Bubble& bubble) {
  // Only change if the name is different
  if (bubble.Name() != browser_->CurrentBubble().Name()) {
    browser_->ChangeCurrentBubble(bubble);
  }
}

const Workspace& PageTreeItem::CurrentWorkspace() {
  auto item = WorkspaceItem();
  if (item) return item->CurrentWorkspace();
  return static_cast<PageTree*>(treeWidget())->ImplicitWorkspace();
}

WorkspaceTreeItem* PageTreeItem::WorkspaceItem() {
  auto par = parent();
  if (!par) return nullptr;
  if (par->type() == PageTree::kPageItemType) {
    return static_cast<PageTreeItem*>(par)->WorkspaceItem();
  }
  return static_cast<WorkspaceTreeItem*>(par);
}

void PageTreeItem::CollapseStateChanged() {
  if (isExpanded() != workspace_page_.Expanded()) {
    workspace_page_.SetExpanded(isExpanded());
    workspace_page_.Persist();
  }
}

void PageTreeItem::ApplyFavicon(const QIcon& icon_override) {
  auto tree = static_cast<PageTree*>(treeWidget());
  if (loading_icon_frame_conn_) tree->disconnect(loading_icon_frame_conn_);
  if (browser_) {
    if (browser_->Loading() && !browser_->Suspended()) {
      tree->LoadingIconMovie()->start();
      // We don't like constantly setting the icon here, but there's not
      // a much better option. Putting a QMovie as a setItemWidget has other
      // display problems and I'm too lazy to create an item delegate.
      loading_icon_frame_conn_ = tree->connect(tree->LoadingIconMovie(),
                                               &QMovie::frameChanged,
                                               [this, tree](int) {
        setIcon(0, QIcon(tree->LoadingIconMovie()->currentPixmap()));
      });
    } else {
      auto icon = icon_override.isNull() ?
            browser_->CurrentFavicon() : icon_override;
      // Only update if an icon is not already set
      if (icon.isNull()) {
        icon = Util::CachedIcon(":/res/images/fontawesome/file-o.png");
      }
      setIcon(0, icon);
    }
  }
}

}  // namespace doogie
