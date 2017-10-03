#ifndef DOOGIE_PROFILE_SETTINGS_DIALOG_H_
#define DOOGIE_PROFILE_SETTINGS_DIALOG_H_

#include <QtWidgets>

#include "blocker_list.h"
#include "bubble.h"
#include "profile.h"

namespace doogie {

// Dialog to edit profile settings.
class ProfileSettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ProfileSettingsDialog(const Cef& cef,
                                 QSet<qlonglong> in_use_bubble_ids,
                                 QWidget* parent = nullptr);
  void SetCurrentTab(int index) { tabs_->setCurrentIndex(index); }
  bool NeedsRestart() const { return needs_restart_; }
  void TryAddBlockerListUrl(const QString& url) {
    try_add_blocker_list_url_(url);
  }
  void done(int r) override;

 signals:
  void BlockerUrlLoadComplete(BlockerList list, bool ok);

 protected:
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  // Needs to be executed in a transaction
  bool Save();
  bool SettingsChanged() const;
  bool BlockerChanged() const;

  QWidget* CreateSettingsTab();
  QWidget* CreateShortcutsTab();
  QWidget* CreateBubblesTab();
  QWidget* CreateBlockerTab(const Cef& cef);
  void AddBlockerListUrl(const Cef& cef, const QString& default_url);

  QTabWidget* tabs_;
  QSet<qlonglong> in_use_bubble_ids_;
  Profile profile_;
  bool needs_restart_ = false;
  QList<Bubble> temp_bubbles_;

  QList<BlockerList> orig_blocker_lists_;
  // Keyed by url or local path
  QHash<QString, BlockerList> temp_blocker_lists_;
  // Value is url or local path
  QSet<QString> enabled_blocker_lists_;
  QList<qlonglong> blocker_list_ids_pending_refresh_;
  std::function<void(const QString&)> try_add_blocker_list_url_;

 signals:
  void Changed();
};

}  // namespace doogie

#endif  // DOOGIE_PROFILE_SETTINGS_DIALOG_H_
