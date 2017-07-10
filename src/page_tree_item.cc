#include "page_tree_item.h"
#include "page_tree.h"

PageTreeItem::PageTreeItem(QPointer<BrowserWidget> browser)
    : browser_(browser) {
  setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
  setText(0, "(New Window)");
  // Connect title and favicon change
  browser->connect(browser, &BrowserWidget::TitleChanged,
                   [this](const QString &title) {
    setText(0, title);
  });
  browser->connect(browser, &BrowserWidget::FaviconChanged,
                   [this](const QIcon &icon) {
    // If the icon is empty, we need to just put a white one there for now
    if (icon.isNull()) {
      QPixmap whiteMap(16, 16);
      whiteMap.fill();
      setIcon(0, QIcon(whiteMap));
    } else {
      setIcon(0, icon);
    }
  });
}

PageTreeItem::~PageTreeItem() {
  // TODO: do I need to delete the close button?
}

QPointer<BrowserWidget> PageTreeItem::Browser() {
  return browser_;
}

QToolButton* PageTreeItem::CloseButton() {
  return close_button_;
}

void PageTreeItem::AfterAdded() {
  close_button_ = new QToolButton();
  close_button_->setCheckable(true);
  close_button_->setIcon(QIcon(":/res/images/fontawesome/times.png"));
  close_button_->setText("Close");
  close_button_->setAutoRaise(true);
  close_button_->setStyleSheet(":pressed:!checked { background-color: transparent; }");
  // Yes, we know we can override one already set, on purpose. When moving
  // items, Qt deletes it (I think, TODO: check life span and ownership of this
  // button as these items are moved).
  treeWidget()->connect(close_button_, &QAbstractButton::clicked, [this](bool) {
    emit ((PageTree*)treeWidget())->ItemClose(this);
  });
  treeWidget()->connect(close_button_, &QAbstractButton::pressed, [this]() {
    emit ((PageTree*)treeWidget())->ItemClosePress(this);
  });
  treeWidget()->connect(close_button_, &QAbstractButton::released, [this]() {
    emit ((PageTree*)treeWidget())->ItemCloseRelease(this);
  });
  treeWidget()->setItemWidget(this, 1, close_button_);
  // Gotta call it on my children too sadly
  for (int i = 0; i < childCount(); i++) {
    ((PageTreeItem*)child(i))->AfterAdded();
  }
}
