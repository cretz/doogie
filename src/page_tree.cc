#include "page_tree.h"

PageTree::PageTree(BrowserStack *browser_stack, QWidget *parent)
    : QDockWidget("Pages", parent),
      browser_stack_(browser_stack) {
  active_item_ = nullptr;
  setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setFeatures(QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable);

  // Create tree
  tree_ = new QTreeWidget(this);
  tree_->setColumnCount(1);
  tree_->setHeaderHidden(true);
  tree_->setCursor(Qt::PointingHandCursor);
  //tree_->setRootIsDecorated(false);
  tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tree_->setDragDropMode(QAbstractItemView::InternalMove);
  tree_->setDefaultDropAction(Qt::MoveAction);
  tree_->setDragEnabled(true);
  tree_->setDropIndicatorShown(true);
  tree_->setAcceptDrops(true);

  // Add the add page button on the bottom
  auto add_page_button_item = new QTreeWidgetItem();
  add_page_button_item->setFlags(Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
  tree_->insertTopLevelItem(0, add_page_button_item);
  auto add_page_button = new QPushButton("New Page...");
  add_page_button->setFlat(true);
  add_page_button->setStyleSheet("text-align: left; color: blue; font-weight: bold;");
  tree_->setItemWidget(add_page_button_item, 0, add_page_button);
  connect(add_page_button, &QAbstractButton::clicked, [this]() {
    auto browser = browser_stack_->NewBrowser("");
    AddBrowser(browser, nullptr, true);
    browser->FocusUrlEdit();
  });

  // Each time one is selected, we need to make sure to show that on the stack
  connect(tree_, &QTreeWidget::currentItemChanged,
          [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
    // Deactivate previous
    if (previous) {
      auto old_font = previous->font(0);
      old_font.setBold(false);
      previous->setFont(0, old_font);
    }
    // Just ignore it if it's the last top level item
    if (tree_->indexOfTopLevelItem(current) != tree_->topLevelItemCount() - 1) {
      auto page_item = static_cast<PageTreeItem*>(current);
      browser_stack_->setCurrentWidget(page_item->Browser());
      auto new_font = page_item->font(0);
      new_font.setBold(true);
      page_item->setFont(0, new_font);
    }
  });

  setWidget(tree_);
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
    tree_->insertTopLevelItem(tree_->topLevelItemCount() - 1, browser_item);
  }
  // Whether to set it as the current active
  if (make_current) {
    tree_->setCurrentItem(browser_item);
    browser->FocusBrowser();
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
  });
}
