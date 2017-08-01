#ifndef DOOGIE_BUBBLE_SETTINGS_DIALOG_H_
#define DOOGIE_BUBBLE_SETTINGS_DIALOG_H_

#include <QtWidgets>
#include "bubble.h"

namespace doogie {

class BubbleSettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit BubbleSettingsDialog(Bubble* bubble,
                                QStringList invalid_names,
                                QWidget* parent = nullptr);

 private:
  QLayoutItem* CreateNameSection();
  QLayoutItem* CreateIconSection();

  Bubble* bubble_;
  QStringList invalid_names_;
  QPushButton* ok_;
  QPushButton* cancel_;
};

}  // namespace doogie

#endif  // DOOGIE_BUBBLE_SETTINGS_DIALOG_H_
