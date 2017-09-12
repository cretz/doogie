#ifndef DOOGIE_BUBBLE_SETTINGS_DIALOG_H_
#define DOOGIE_BUBBLE_SETTINGS_DIALOG_H_

#include <QtWidgets>

#include "bubble.h"

namespace doogie {

// Dialog for creating/editing a bubble.
class BubbleSettingsDialog : public QDialog {
  Q_OBJECT

 public:
  // This saves the profile/bubble on success and returns the name
  // or -1 on failure
  static qlonglong NewBubble(QWidget* parent = nullptr);
  // This does not save anything and just updates the reference
  static bool UpdateBubble(Bubble* bubble,
                           QSet<QString> invalid_names,
                           QWidget* parent = nullptr);

  void done(int r) override;

 protected:
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  explicit BubbleSettingsDialog(Bubble* bubble,
                                QSet<QString> invalid_names,
                                QWidget* parent = nullptr);
  QLayoutItem* CreateNameSection();
  QLayoutItem* CreateIconSection();
  QLayoutItem* CreateSettingsSection();

  Bubble* orig_bubble_;
  Bubble bubble_;
  QSet<QString> invalid_names_;

 signals:
  void Changed();
};

}  // namespace doogie

#endif  // DOOGIE_BUBBLE_SETTINGS_DIALOG_H_
