#include "page_tree_item.h"
#include "page_tree.h"

PageTreeItem::PageTreeItem(QPointer<BrowserWidget> browser)
    : browser_(browser) {
  setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
  // Connect title and favicon change
  setText(0, "(New Window)");
  browser->connect(browser, &BrowserWidget::TitleChanged, [this]() {
    setText(0, browser_->CurrentTitle());
  });
  browser->connect(browser, &BrowserWidget::LoadingStateChanged,
                   [this]() { ApplyFavicon(); });
  browser->connect(browser, &BrowserWidget::FaviconChanged,
                   [this]() { ApplyFavicon(); });
}

PageTreeItem::~PageTreeItem() {
  // TODO: do I need to delete the widgets?
}

QPointer<BrowserWidget> PageTreeItem::Browser() {
  return browser_;
}

QToolButton* PageTreeItem::CloseButton() {
  return close_button_;
}

void PageTreeItem::AfterAdded() {
  auto tree = (PageTree*) treeWidget();
  ApplyFavicon();

  // Create a new close button everytime because it is destroyed otherwise
  close_button_ = new QToolButton();
  close_button_->setCheckable(true);
  close_button_->setIcon(tree->CloseButtonIcon());
  close_button_->setText("Close");
  close_button_->setAutoRaise(true);
  close_button_->setStyleSheet(":pressed:!checked { background-color: transparent; }");
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
  tree->setItemWidget(this, 1, close_button_);
  // Gotta call it on my children too sadly
  for (int i = 0; i < childCount(); i++) {
    ((PageTreeItem*)child(i))->AfterAdded();
  }
}

void PageTreeItem::ApplyFavicon() {
  auto tree = (PageTree*) treeWidget();
  if (loading_icon_frame_conn_) tree->disconnect(loading_icon_frame_conn_);
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
      QPixmap whiteMap(16, 16);
      whiteMap.fill();
      icon = QIcon(whiteMap);
    }
    setIcon(0, icon);
  }
}
