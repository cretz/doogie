#ifndef DOOGIE_PROFILE_H_
#define DOOGIE_PROFILE_H_

#include <QtWidgets>

#include "action_manager.h"
#include "browser_setting.h"
#include "cef/cef.h"
#include "util.h"
#include "workspace.h"

namespace doogie {

// DB model and utilities for profiles.
class Profile {
 public:
  static const QString kInMemoryPath;
  static const QString kAppDataPath;
  static const QString kDoogieAppDataPath;

  static Profile& Current() { return current_; }

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

  static QStringList LastTenProfilePaths();

  // This must be current for this to work.
  bool Persist();
  // This must be current for this to work.
  bool Reload();
  void CopySettingsFrom(const Profile& other);

  QString Path() const { return path_; }
  QString FriendlyName() const { return FriendlyName(path_); }
  bool InMemory() const { return path_ == kInMemoryPath; }

  // Null means default, empty means no cache
  QString CachePath() const { return cache_path_; }
  void SetCachePath(const QString& cache_path) { cache_path_ = cache_path; }
  QString UserAgent() const { return user_agent_; }
  void SetUserAgent(const QString& user_agent) { user_agent_ = user_agent; }
  // Null means Doogie default, empty means platform default
  QString UserDataPath() const { return user_data_path_; }
  void SetUserDataPath(const QString& path) { user_data_path_ = path; }
  QHash<BrowserSetting::SettingKey, bool> BrowserSettings() const {
    return browser_settings_;
  }
  void SetBrowserSettings(
      const QHash<BrowserSetting::SettingKey, bool>& browser_settings) {
    browser_settings_ = browser_settings;
  }

  // Keyed by action type
  QHash<int, QList<QKeySequence>> Shortcuts() const {
    return shortcuts_;
  }
  void SetShortcuts(QHash<int, QList<QKeySequence>> shortcuts) {
    shortcuts_ = shortcuts;
  }

  QSet<qlonglong> EnabledBlockerListIds() const {
    return enabled_blocker_list_ids_;
  }
  void SetEnabledBlockerListIds(QSet<qlonglong> enabled_blocker_list_ids) {
    enabled_blocker_list_ids_ = enabled_blocker_list_ids;
  }

  // TODO(cretz): IsOpenElsewhere - check PID on QSharedMemory and if
  //  still open, this returns true

  void ApplyCefSettings(CefSettings* settings) const;
  void ApplyCefBrowserSettings(CefBrowserSettings* settings) const;
  void ApplyCefRequestContextSettings(
      CefRequestContextSettings* settings) const;

  void ApplyActionShortcuts() const;

  bool RequiresRestartIfChangedTo(const Profile& other) const;
  bool operator==(const Profile& other) const;
  bool operator!=(const Profile& other) const { return !operator==(other); }

 private:
  static bool SetCurrent(const Profile& profile);

  explicit Profile(const QString& path);
  void ApplyDefaults();

  static Profile current_;

  QString path_;
  QString cache_path_;
  QString user_agent_;
  QString user_data_path_;
  QHash<BrowserSetting::SettingKey, bool> browser_settings_;
  QHash<int, QList<QKeySequence>> shortcuts_;
  QSet<qlonglong> enabled_blocker_list_ids_;
};
\
}  // namespace doogie

#endif  // DOOGIE_PROFILE_H_
