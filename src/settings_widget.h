#ifndef DOOGIE_SETTINGS_WIDGET_H_
#define DOOGIE_SETTINGS_WIDGET_H_

#include <QtWidgets>

namespace doogie {

// Scrollable widget area for adding browser settings.
class SettingsWidget : public QScrollArea {
  Q_OBJECT
 public:
  explicit SettingsWidget(QWidget* parent = nullptr);

  void AddSetting(const QString& name, const QString& desc, QWidget* widg);
  QComboBox* AddComboBoxSetting(const QString& name,
                                const QString& desc,
                                QStringList items,
                                int selected);
  QComboBox* AddYesNoSetting(const QString& name,
                             const QString& desc,
                             bool curr,
                             bool default_val);
  void AddSettingBreak();

 private:
  QGridLayout* layout_;
};

}  // namespace doogie

#endif  // DOOGIE_SETTINGS_WIDGET_H_
