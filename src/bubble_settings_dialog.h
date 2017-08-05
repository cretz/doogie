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
  static Bubble* NewBubble(QWidget* parent = nullptr);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  QLayoutItem* CreateNameSection();
  QLayoutItem* CreateIconSection();
  QLayoutItem* CreateSettingsSection();
  void CheckSettingsChanged();

  Bubble* bubble_;
  QJsonObject orig_prefs_;
  QStringList invalid_names_;
  QPushButton* ok_;
  QPushButton* cancel_;
  bool settings_changed_;

 signals:
  void SettingsChangedUpdated();
};

}  // namespace doogie

#endif  // DOOGIE_BUBBLE_SETTINGS_DIALOG_H_
