#include "page_close_button.h"

#include "util.h"

namespace doogie {

PageCloseButton::PageCloseButton(QTreeWidget* tree, QWidget* parent) :
    QToolButton(parent), tree_(tree) {
  setCheckable(true);
  setIcon(Util::CachedIcon(":/res/images/fontawesome/times.png"));
  setText("Close");
  setAutoRaise(true);
}

void PageCloseButton::mouseMoveEvent(QMouseEvent*) {
  if (close_dragging_) {
    auto button = qobject_cast<PageCloseButton*>(
          tree_->childAt(tree_->mapFromGlobal(QCursor::pos())));
    if (button && last_seen_ != button) {
      button->setChecked(!button->isChecked());
      button->setDown(button->isChecked());
    }
    last_seen_ = button;
  }
}

void PageCloseButton::mousePressEvent(QMouseEvent*) {
  close_dragging_ = true;
  last_seen_ = this;
  setChecked(true);
  setDown(true);
  setStyleSheet(":hover:!checked { background-color: transparent; }");
}

void PageCloseButton::mouseReleaseEvent(QMouseEvent*) {
  close_dragging_ = false;
  last_seen_ = nullptr;
  setStyleSheet("");
  emit Released();
}

}  // namespace doogie
