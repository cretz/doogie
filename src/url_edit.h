#ifndef DOOGIE_URL_EDIT_H_
#define DOOGIE_URL_EDIT_H_

#include <QtWidgets>

namespace doogie {

class UrlEdit : public QLineEdit {
  Q_OBJECT
 public:
  explicit UrlEdit(QWidget* parent);

 protected:
  void focusOutEvent(QFocusEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  void HandleTextEdit(const QString& text);

  QListWidget* autocomplete_;
  QString typed_before_moved_;
};

}  // namespace doogie

#endif  // DOOGIE_URL_EDIT_H_
