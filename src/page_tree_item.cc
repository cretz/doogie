#include "page_tree_item.h"
#include "page_tree.h"
#include "util.h"

namespace doogie {

PageTreeItem::PageTreeItem(QPointer<BrowserWidget> browser)
    : QTreeWidgetItem(PageTree::kPageItemType), browser_(browser) {
  setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
  // Connect title and favicon change
  setText(0, "(New Window)");
  browser->connect(browser, &BrowserWidget::TitleChanged, [this]() {
    if (browser_) {
      setText(0, browser_->CurrentTitle());
      setToolTip(0, browser_->CurrentTitle());
    }
  });
  browser->connect(browser, &BrowserWidget::LoadingStateChanged,
                   [this]() { ApplyFavicon(); });
  browser->connect(browser, &BrowserWidget::FaviconChanged,
                   [this]() { ApplyFavicon(); });
  browser->connect(browser, &BrowserWidget::destroyed, [this]() {
    // Move all the children up
    if (parent()) {
      parent()->insertChildren(parent()->indexOfChild(this), takeChildren());
    } else {
      treeWidget()->insertTopLevelItems(
            treeWidget()->indexOfTopLevelItem(this), takeChildren());
    }
    delete this;
  });
  browser->connect(browser, &BrowserWidget::AboutToShowJSDialog,
                   [this]() {
    treeWidget()->setCurrentItem(this);
  });
  browser->connect(browser, &BrowserWidget::CloseCancelled, [this]() {
    close_button_->setChecked(false);
  });
  browser->connect(browser, &BrowserWidget::SuspensionChanged, [this]() {
    auto palette = QGuiApplication::palette();
    if (browser_->Suspended()) {
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
  });
  browser->connect(browser, &BrowserWidget::BubbleMaybeChanged, [=]() {
    auto label = qobject_cast<QLabel*>(
          treeWidget()->itemWidget(this, kBubbleIconColumn));
    if (label) {
      label->setPixmap(browser_->CurrentBubble()->Icon().pixmap(16, 16));
      label->setToolTip("Bubble: " + browser_->CurrentBubble()->FriendlyName());
    }
  });
}

PageTreeItem::~PageTreeItem() {
  if (loading_icon_frame_conn_) {
    treeWidget()->disconnect(loading_icon_frame_conn_);
  }
}

QPointer<BrowserWidget> PageTreeItem::Browser() {
  return browser_;
}

QToolButton* PageTreeItem::CloseButton() {
  return close_button_;
}

void PageTreeItem::AfterAdded() {
  auto tree = static_cast<PageTree*>(treeWidget());
  ApplyFavicon();

  // Set the icon
  auto label = new QLabel;
  label->setFixedWidth(16);
  label->setAlignment(Qt::AlignCenter);
  label->setPixmap(browser_->CurrentBubble()->Icon().pixmap(16, 16));
  label->setToolTip("Bubble: " + browser_->CurrentBubble()->FriendlyName());
  tree->setItemWidget(this, kBubbleIconColumn, label);
  label->setContextMenuPolicy(Qt::CustomContextMenu);
  tree->connect(label, &QLabel::customContextMenuRequested, [=]() {
    QMenu menu;
    tree->ApplyBubbleSelectMenu(&menu, { this });
    menu.exec(label->mapToGlobal(label->rect().bottomLeft()));
  });

  // Create a new close button everytime because it is destroyed otherwise
  close_button_ = new QToolButton();
  close_button_->setCheckable(true);
  close_button_->setIcon(
        Util::CachedIcon(":/res/images/fontawesome/times.png"));
  close_button_->setText("Close");
  close_button_->setAutoRaise(true);
  close_button_->setStyleSheet(
        ":pressed:!checked { background-color: transparent; }");
  // Yes, we know we can override one already set, on purpose. When moving
  // items, Qt deletes it (I think, TODO: check life span and ownership of this
  // button as these items are moved).
  tree->connect(close_button_, &QAbstractButton::clicked, [this, tree](bool) {
    emit tree->ItemClose(this);
  });
  tree->connect(close_button_, &QAbstractButton::pressed, [this, tree]() {
    emit tree->ItemClosePress(this);
  });
  tree->connect(close_button_, &QAbstractButton::released, [this, tree]() {
    emit tree->ItemCloseRelease(this);
  });
  tree->setItemWidget(this, kCloseButtonColumn, close_button_);
  // Gotta call it on my children too sadly
  for (int i = 0; i < childCount(); i++) {
    static_cast<PageTreeItem*>(child(i))->AfterAdded();
  }
}

QJsonObject PageTreeItem::DebugDump() {
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

bool PageTreeItem::SelfOrAnyChildCollapsed() {
  for (int i = 0; i < childCount(); i++) {
    if (static_cast<PageTreeItem*>(child(i))->SelfOrAnyChildCollapsed()) {
      return true;
    }
  }
  return false;
}

bool PageTreeItem::SelfOrAnyChildExpanded() {
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

QList<PageTreeItem*> PageTreeItem::SelfSelectedOrChildrenSelected() {
  if (isSelected()) return { this };
  QList<PageTreeItem*> ret;
  for (int i = 0; i < childCount(); i++) {
    ret.append(static_cast<PageTreeItem*>(child(i))->
        SelfSelectedOrChildrenSelected());
  }
  return ret;
}

bool PageTreeItem::SelectedOrHasSelectedParent() {
  if (isSelected()) return true;
  auto my_parent = static_cast<PageTreeItem*>(parent());
  if (!my_parent) return false;
  return my_parent->SelectedOrHasSelectedParent();
}

QList<PageTreeItem*> PageTreeItem::Siblings() {
  auto my_parent = parent();
  if (!my_parent) my_parent = treeWidget()->invisibleRootItem();
  QList<PageTreeItem*> ret;
  ret.reserve(my_parent->childCount() - 1);
  for (int i = 0; i < my_parent->childCount(); i++) {
    ret.append(static_cast<PageTreeItem*>(my_parent->child(i)));
  }
  return ret;
}

void PageTreeItem::SetCurrentBubbleIfDifferent(Bubble *bubble) {
  // Only change if the name is different
  if (bubble->Name() != browser_->CurrentBubble()->Name()) {
    browser_->ChangeCurrentBubble(bubble);
  }
}

void PageTreeItem::ApplyFavicon() {
  auto tree = static_cast<PageTree*>(treeWidget());
  if (loading_icon_frame_conn_) tree->disconnect(loading_icon_frame_conn_);
  if (browser_) {
    if (browser_->Loading()) {
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
      auto icon = browser_->CurrentFavicon();
      if (icon.isNull()) {
        icon = Util::CachedIcon(":/res/images/fontawesome/file-o.png");
      }
      setIcon(0, icon);
    }
  }
}

}  // namespace doogie
