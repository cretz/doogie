#ifndef DOOGIE_PROFILE_SETTINGS_DIALOG_H_
#define DOOGIE_PROFILE_SETTINGS_DIALOG_H_

#include <QtWidgets>

#include "bubble.h"
#include "profile.h"

namespace doogie {

class ProfileSettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ProfileSettingsDialog(Profile* profile,
                                 QSet<QString> in_use_bubble_names,
                                 QWidget* parent = nullptr);
  bool NeedsRestart() const { return needs_restart_; }
  void done(int r) override;

 protected:
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  QWidget* CreateSettingsTab();
  QWidget* CreateShortcutsTab();
  QWidget* CreateBubblesTab();

  Profile* orig_profile_;
  QSet<QString> in_use_bubble_names_;
  Profile profile_;
  bool needs_restart_ = false;
  QList<Bubble> temp_bubbles_;

 signals:
  void Changed();
  void SettingsChangedUpdated();
  void ShortcutsChangedUpdated();
  void BubblesChangedUpdated();
};

}  // namespace doogie

#endif  // DOOGIE_PROFILE_SETTINGS_DIALOG_H_
