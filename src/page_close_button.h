#ifndef DOOGIE_PAGE_CLOSE_BUTTON_H_
#define DOOGIE_PAGE_CLOSE_BUTTON_H_

#include <QtWidgets>

namespace doogie {

class PageCloseButton : public QToolButton {
  Q_OBJECT

 public:
  explicit PageCloseButton(QTreeWidget* tree, QWidget* parent = nullptr);

 signals:
  void Released();

 protected:
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

 private:
  QTreeWidget* tree_;
  PageCloseButton* last_seen_ = nullptr;
  bool close_dragging_ = false;
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_CLOSE_BUTTON_H_
