#ifndef DOOGIE_FIND_WIDGET_H_
#define DOOGIE_FIND_WIDGET_H_

#include <QtWidgets>

namespace doogie {

class FindWidget : public QFrame {
  Q_OBJECT

 public:
  explicit FindWidget(QWidget* parent);

 public slots:
  void FindResult(int count, int index);

 signals:
  void AttemptFind(const QString& text,
                   bool forward,
                   bool match_case,
                   bool continued);
  void Hidden();

 protected:
  virtual void focusInEvent(QFocusEvent* event) override;
  virtual void hideEvent(QHideEvent* event) override;
  virtual void keyReleaseEvent(QKeyEvent* event) override;
  virtual void showEvent(QShowEvent* event) override;

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
