#ifndef DOOGIE_FIND_WIDGET_H_
#define DOOGIE_FIND_WIDGET_H_

#include <QtWidgets>

namespace doogie {

// Widget for showing the find-in-page feature.
class FindWidget : public QFrame {
  Q_OBJECT

 public:
  explicit FindWidget(QWidget* parent);
  void FindResult(int count, int index);

 signals:
  void AttemptFind(const QString& text,
                   bool forward,
                   bool match_case,
                   bool continued);
  void Hidden();

 protected:
  void focusInEvent(QFocusEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void showEvent(QShowEvent* event) override;

 private:
  void UpdateFindResults(int count,
                         int index,
                         bool real_search);

  QLineEdit* find_text_;
  QLabel* find_results_;
  bool unchanged_means_continue_ = true;
};

}  // namespace doogie

#endif  // DOOGIE_FIND_WIDGET_H_
