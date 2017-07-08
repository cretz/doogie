#include "page_tree.h"

PageTree::PageTree(BrowserStack *browser_stack, QWidget *parent)
    : QTreeWidget(parent), browser_stack_(browser_stack) {
  setColumnCount(1);
  setHeaderHidden(true);
  setCursor(Qt::PointingHandCursor);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setDragDropMode(QAbstractItemView::InternalMove);
  setDefaultDropAction(Qt::MoveAction);
  setDragEnabled(true);
  setDropIndicatorShown(true);
  setAcceptDrops(true);
  setAutoExpandDelay(500);
  setIndentation(10);
  setAnimated(true);

  // Each time one is selected, we need to make sure to show that on the stack
  connect(this, &QTreeWidget::currentItemChanged,
          [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
    // Deactivate previous
    if (previous) {
      auto old_font = previous->font(0);
      old_font.setBold(false);
      previous->setFont(0, old_font);
    }
    // Just ignore it if it's the last top level item
    if (current) {
      auto page_item = static_cast<PageTreeItem*>(current);
      browser_stack_->setCurrentWidget(page_item->Browser());
      auto new_font = page_item->font(0);
      new_font.setBold(true);
      page_item->setFont(0, new_font);
    }
  });
}

void PageTree::NewBrowser() {
  auto browser = browser_stack_->NewBrowser("");
  AddBrowser(browser, nullptr, true);
  browser->FocusUrlEdit();
}

Qt::DropActions PageTree::supportedDropActions() const {
  // return Qt::MoveAction;
  return QTreeWidget::supportedDropActions();
}

void PageTree::dropEvent(QDropEvent *event) {
  // Due to bad internal Qt logic, we reset the current here
  auto current = currentItem();
  QTreeWidget::dropEvent(event);
  setCurrentItem(current);
}

void PageTree::AddBrowser(QPointer<BrowserWidget> browser,
                          PageTreeItem *parent,
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
    setCurrentItem(browser_item);
  }
  // Make all tab opens open as child
  connect(browser, &BrowserWidget::TabOpen,
          [this, browser_item](BrowserWidget::WindowOpenType type,
                               const QString &url,
                               bool user_gesture) {
    PageTreeItem *parent = nullptr;
    bool make_current = user_gesture && type != BrowserWidget::OpenTypeNewBackgroundTab;
    if (type != BrowserWidget::OpenTypeNewWindow) {
      parent = browser_item;
    }
    AddBrowser(browser_stack_->NewBrowser(url), parent, make_current);
    if (make_current) browser_item->Browser()->FocusBrowser();
  });
}
