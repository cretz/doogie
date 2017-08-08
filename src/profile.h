#ifndef DOOGIE_PROFILE_H_
#define DOOGIE_PROFILE_H_

#include <QtWidgets>

#include "action_manager.h"
#include "bubble.h"
#include "cef/cef.h"
#include "util.h"
#include "workspace.h"

namespace doogie {

class Profile {
 public:
  struct BrowserSetting {
    QString name;
    QString desc;
    QString field;
  };

  static const QString kInMemoryPath;

  static Profile* Current() { return current_; }
  // False on failure
  static bool CreateOrLoadProfile(const QString& path,
                                  bool set_current = true);
  // False on failure. As a special case, an empty
  // or null string creates an in-mem-only profile
  static bool CreateProfile(const QString& path, bool set_current = true);
  // False on failure
  static bool LoadProfile(const QString& path, bool set_current = true);
  // False on failure
  static bool LoadProfileFromCommandLine(int argc, char* argv[]);
  // False on failure
  static bool LaunchWithProfile(const QString& profile_path);

  static QString FriendlyName(const QString& path);
  static QList<BrowserSetting> PossibleBrowserSettings();

  static QStringList LastTenProfilePaths();

  Profile(const QString& path = kInMemoryPath,
          const QJsonObject& obj = QJsonObject());

  // Must be called after Application is constructed
  void Init();

  void CopySettingsFrom(const Profile& profile);

  QString Path() const { return path_; }
  QString FriendlyName() const { return FriendlyName(path_); }
  bool InMemory() const { return path_ == kInMemoryPath; }

  // Null means default, empty means no cache
  QString CachePath() const { return cache_path_; }
  void SetCachePath(const QString& cache_path) { cache_path_ = cache_path; }
  bool EnableNetSec() { return enable_net_sec_; }
  void SetEnableNetSec(bool enabled) { enable_net_sec_ = enabled; }
  bool PersistUserPrefs() { return persist_user_prefs_; }
  void SetPersistUserPrefs(bool enabled) { persist_user_prefs_ = enabled; }
  QString UserAgent() const { return user_agent_; }
  void SetUserAgent(const QString& user_agent) { user_agent_ = user_agent; }
  // Null means Doogie default, empty means platform default
  QString UserDataPath() const { return user_data_path_; }
  void SetUserDataPath(const QString& path) { user_data_path_ = path; }
  Util::SettingState GetBrowserSetting(const QString& field) const {
    if (!browser_settings_.contains(field)) return Util::Default;
    return browser_settings_[field] ? Util::Enabled : Util::Disabled;
  }
  void SetBrowserSetting(const QString& field, Util::SettingState state) {
    if (state == Util::Default) {
      browser_settings_.remove(field);
    } else {
      browser_settings_[field] = state == Util::Enabled;
    }
  }
  const QList<Bubble> Bubbles() const { return bubbles_; }
  // Note, this will reuse bubble instances of the same name
  void SetBubbles(QList<Bubble> bubbles) { bubbles_ = bubbles; }
  const Bubble& DefaultBubble() const { return bubbles_.first(); }
  int BubbleIndexFromName(const QString& name) const;

  // Keyed by action type
  const QHash<int, QList<QKeySequence>> Shortcuts() const {
    return shortcuts_;
  }
  void SetShortcuts(QHash<int, QList<QKeySequence>> shortcuts) {
    shortcuts_ = shortcuts;
  }

  const QList<qlonglong> OpenWorkspaceIds() const {
    return open_workspace_ids_;
  }
  void SetOpenWorkspaceIds(QList<qlonglong> open_workspace_ids) {
    open_workspace_ids_ = open_workspace_ids;
  }

  // TODO(cretz): IsOpenElsewhere - check PID on QSharedMemory and if
  //  still open, this returns true

  void ApplyCefSettings(CefSettings* settings) const;
  void ApplyCefBrowserSettings(CefBrowserSettings* settings) const;

  void ApplyActionShortcuts() const;

  QJsonObject ToJson() const;
  bool SavePrefs() const;

  bool RequiresRestartIfChangedTo(const Profile& other) const;
  bool operator==(const Profile& other) const;
  bool operator!=(const Profile& other) const { return !operator==(other); }

 private:
  static const QString kAppDataPath;

  static bool SetCurrent(Profile* profile);

  static Profile* current_;
  static QList<BrowserSetting> possible_browser_settings_;

  QString path_;
  QString cache_path_;
  bool enable_net_sec_;
  bool persist_user_prefs_;
  QString user_agent_;
  QString user_data_path_;
  QHash<QString, bool> browser_settings_;
  QList<Bubble> bubbles_;
  QHash<int, QList<QKeySequence>> shortcuts_;
  QList<qlonglong> open_workspace_ids_;
};
\
}  // namespace doogie

#endif  // DOOGIE_PROFILE_H_
