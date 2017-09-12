#ifndef DOOGIE_PROFILE_CHANGE_DIALOG_H_
#define DOOGIE_PROFILE_CHANGE_DIALOG_H_

#include <QtWidgets>

namespace doogie {

// Dialog to switch profiles.
class ProfileChangeDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ProfileChangeDialog(QWidget* parent = nullptr);
  bool NeedsRestart() const;
  QString ChosenPath() const;
  void done(int r) override;

 private:
  QComboBox* selector_;
  bool needs_restart_;
};

}  // namespace doogie

#endif  // DOOGIE_PROFILE_CHANGE_DIALOG_H_
