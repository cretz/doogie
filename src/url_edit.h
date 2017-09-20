#ifndef DOOGIE_URL_EDIT_H_
#define DOOGIE_URL_EDIT_H_

#include <QtWidgets>
#include "cef/cef.h"

namespace doogie {

// URL edit input.
class UrlEdit : public QLineEdit {
  Q_OBJECT
 public:
  explicit UrlEdit(const Cef& cef, QWidget* parent);

 signals:
  void UrlEntered();

 protected:
  void focusInEvent(QFocusEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  static const int kRoleAutocompleteUrl = Qt::UserRole + 1;

  void HandleTextEdit(const QString& text);

  const Cef& cef_;
  QListWidget* autocomplete_;
  QString typed_before_moved_;
};

}  // namespace doogie

#endif  // DOOGIE_URL_EDIT_H_
