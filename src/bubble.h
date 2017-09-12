#ifndef DOOGIE_BUBBLE_H_
#define DOOGIE_BUBBLE_H_

#include <QtSql>
#include <QtWidgets>

#include "browser_setting.h"
#include "cef/cef.h"
#include "util.h"

namespace doogie {

class Profile;

// DB model for a bubble.
class Bubble {
 public:
  static Bubble DefaultBubble();
  static QList<Bubble> CachedBubbles();
  static Bubble FromId(qlonglong id, bool* ok = nullptr);
  static void InvalidateCachedBubbles();

  // Essentially turns all order indices negative in the DB
  // so they can each be updated individually without violating
  // the unique constraint.
  static bool ResetOrderIndexes();

  Bubble();

  bool Persist();
  bool Delete();
  bool Reload();
  void CopySettingsFrom(const Bubble& other);

  qlonglong Id() const { return id_; }
  bool Exists() const { return id_ >= 0; }
  int OrderIndex() const { return order_index_; }
  void SetOrderIndex(int order_index) { order_index_ = order_index; }
  QString Name() const { return name_; }
  void SetName(const QString& name) { name_ = name; }
  QString FriendlyName() const {
    return name_.isEmpty() ? "(default)" : name_;
  }
  QIcon Icon() const { return icon_; }
  // Note, null means none, empty means default
  QString IconPath() const { return icon_path_; }
  void SetIconPath(const QString& path) {
    if (path != icon_path_ || path.isNull() != icon_path_.isNull()) {
      icon_path_ = path;
      RebuildIcon();
    }
  }
  QColor IconColor() const { return icon_color_; }
  void SetIconColor(const QColor& color) {
    if (color != icon_color_) {
      icon_color_ = color;
      RebuildIcon();
    }
  }
  // Note, null means default, empty means none
  QString CachePath() const { return cache_path_; }
  void SetCachePath(const QString& cache_path) { cache_path_ = cache_path; }
  QHash<BrowserSetting::SettingKey, bool> BrowserSettings() const {
    return browser_settings_;
  }
  void SetBrowserSettings(
      const QHash<BrowserSetting::SettingKey, bool>& browser_settings) {
    browser_settings_ = browser_settings;
  }
  bool OverridesEnabledBlockerLists() const {
    return overrides_blocker_lists_;
  }
  void SetOverridesEnabledBlockerLists(bool overrides_blocker_lists) {
    overrides_blocker_lists_ = overrides_blocker_lists;
  }
  QSet<qlonglong> EnabledBlockerListIds() const {
    return enabled_blocker_list_ids_;
  }
  void SetEnabledBlockerListIds(QSet<qlonglong> enabled_blocker_list_ids) {
    enabled_blocker_list_ids_ = enabled_blocker_list_ids;
  }

  void ApplyCefBrowserSettings(CefBrowserSettings* settings,
                               bool include_current_profile = true) const;
  void ApplyCefRequestContextSettings(
      CefRequestContextSettings* settings,
      bool include_current_profile = true) const;
  CefRefPtr<CefRequestContext> CreateCefRequestContext() const;

  bool operator==(const Bubble& other) const;
  bool operator!=(const Bubble& other) const { return !operator==(other); }

 private:
  friend class Profile;

  static bool PersistBrowserSettings(
      const QHash<BrowserSetting::SettingKey, bool>& browser_settings,
      QVariant bubble_id = QVariant());
  static QHash<BrowserSetting::SettingKey, bool> LoadBrowserSettings(
      QVariant bubble_id = QVariant());
  static bool PersistEnabledBlockerListIds(
      const QSet<qlonglong>& enabled_blocker_list_ids,
      QVariant bubble_id = QVariant());
  static QSet<qlonglong> LoadEnabledBlockerListIds(
      QVariant bubble_id = QVariant());
  static void ApplyBrowserSettings(
      const QHash<BrowserSetting::SettingKey, bool>& source,
      CefBrowserSettings* target);
  static void ApplyBrowserSettings(
      const QHash<BrowserSetting::SettingKey, bool>& source,
      CefRequestContextSettings* target);
  static void ApplyBrowserSettings(
      const QHash<BrowserSetting::SettingKey, bool>& source,
      CefSettings* target);

  explicit Bubble(const QSqlRecord& record);

  void ApplySqlRecord(const QSqlRecord& record);
  void RebuildIcon();

  static QList<Bubble> cached_bubbles_;

  qlonglong id_ = -1;
  int order_index_ = -1;
  QString name_;
  QIcon icon_;
  QString icon_path_;
  QColor icon_color_;
  QString cache_path_;
  QHash<BrowserSetting::SettingKey, bool> browser_settings_;
  bool overrides_blocker_lists_ = false;
  QSet<qlonglong> enabled_blocker_list_ids_;
};
\
}  // namespace doogie

#endif  // DOOGIE_BUBBLE_H_
