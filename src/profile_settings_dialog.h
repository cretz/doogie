#ifndef DOOGIE_PROFILE_SETTINGS_DIALOG_H_
#define DOOGIE_PROFILE_SETTINGS_DIALOG_H_

#include <QtWidgets>
#include "profile.h"

namespace doogie {

class ProfileSettingsDialog : public QDialog {
  Q_OBJECT
 public:
  explicit ProfileSettingsDialog(Profile*, QWidget* parent = nullptr);
  bool NeedsRestart();
  void done(int r) override;

 protected:
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  QWidget* CreateSettingsTab();
  QJsonObject BuildCefPrefsJson();
  QJsonObject BuildBrowserPrefsJson();
  void CheckSettingsChange();

  QWidget* CreateShortcutsTab();
  QJsonObject BuildShortcutsPrefsJson();
  void CheckShortcutsChange();

  QJsonObject BuildPrefsJson();

  Profile* profile_;
  QCheckBox* cache_path_disabled_;
  QLineEdit* cache_path_edit_;
  QComboBox* enable_net_sec_;
  QComboBox* user_prefs_;
  QLineEdit* user_agent_edit_;
  QCheckBox* user_data_path_disabled_;
  QLineEdit* user_data_path_edit_;
  QHash<QString, QComboBox*> browser_setting_widgs_;
  QTableWidget* shortcuts_;
  bool settings_changed_ = false;
  bool shortcuts_changed_ = false;

 signals:
  void SettingsChangedUpdated();
  void ShortcutsChangedUpdated();
};

}  // namespace doogie

#endif  // DOOGIE_PROFILE_SETTINGS_DIALOG_H_
