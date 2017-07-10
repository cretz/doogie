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

QPointer<BrowserWidget> PageTreeItem::Browser() {
  return browser_;
}

void PageTreeItem::AfterAdded() {
  // Create the close button
  auto close_button = new QToolButton();
  close_button->setIcon(QIcon(":/res/images/fontawesome/times.png"));
  close_button->setText("Close");
  close_button->setAutoRaise(true);
  treeWidget()->connect(close_button, &QAbstractButton::clicked, [this](bool) {
    emit ((PageTree*)treeWidget())->ItemClose(this);
  });
  treeWidget()->setItemWidget(this, 1, close_button);
}
