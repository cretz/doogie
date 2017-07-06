#include "page_tree.h"

PageTree::PageTree(BrowserStack *browser_stack, QWidget *parent)
    : QDockWidget("Pages", parent),
      browser_stack_(browser_stack) {
  setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setFeatures(QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable);

  // Create tree
  auto tree = new QTreeWidget(this);
  tree->setColumnCount(1);
  tree->setHeaderHidden(true);
  tree->setCursor(Qt::PointingHandCursor);

  // Add the add page button on the bottom
  auto add_page_button = new QTreeWidgetItem((QTreeWidget*)0, QStringList("Add Page..."));
  add_page_button->setTextAlignment(0, Qt::AlignHCenter);
  add_page_button->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsEnabled);
  QFont font(add_page_button->font(0));
  font.setItalic(true);
  font.setBold(true);
  add_page_button->setFont(0, font);
  tree->insertTopLevelItem(0, add_page_button);

  // Click handler
  connect(tree, &QTreeWidget::itemClicked,
          [this, add_page_button](QTreeWidgetItem *item, int column) {
    if (item == add_page_button) {
      browser_stack_->NewBrowser();
    }
  });

  setWidget(tree);
}
