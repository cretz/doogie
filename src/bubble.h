#ifndef DOOGIE_BUBBLE_H_
#define DOOGIE_BUBBLE_H_

#include <QtWidgets>

#include "cef/cef.h"
#include "util.h"

namespace doogie {

class Bubble {
 public:
  explicit Bubble(const QJsonObject& obj = QJsonObject());

  void CopySettingsFrom(const Bubble& other);

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
      emit RebuildIcon();
    }
  }
  QColor IconColor() const { return icon_color_; }
  void SetIconColor(const QColor& color) {
    if (color != icon_color_) {
      icon_color_ = color;
      emit RebuildIcon();
    }
  }

  // Note, null means profile default, empty means none
  QString CachePath() const { return cache_path_; }
  void SetCachePath(const QString& cache_path) { cache_path_ = cache_path; }
  Util::SettingState EnableNetSec() { return enable_net_sec_; }
  void SetEnableNetSec(Util::SettingState state) {
    enable_net_sec_ = state;
  }
  Util::SettingState PersistUserPrefs() { return persist_user_prefs_; }
  void SetPersistUserPrefs(Util::SettingState state) {
    persist_user_prefs_ = state;
  }
  Util::SettingState BrowserSetting(const QString& field) const {
    if (!browser_settings_.contains(field)) return Util::Default;
    return browser_settings_.value(field) ?
          Util::Enabled : Util::Disabled;
  }
  void SetBrowserSetting(const QString& field, Util::SettingState state) {
    if (state == Util::Default) {
      browser_settings_.remove(field);
    } else {
      browser_settings_[field] = state == Util::Enabled;
    }
  }

  void ApplyCefBrowserSettings(CefBrowserSettings* settings,
                               bool include_current_profile = true) const;
  void ApplyCefRequestContextSettings(
      CefRequestContextSettings* settings,
      bool include_current_profile = true) const;
  CefRefPtr<CefRequestContext> CreateCefRequestContext() const;

  QJsonObject ToJson() const;

  bool operator==(const Bubble& other) const;
  bool operator!=(const Bubble& other) const { return !operator==(other); }

 private:
  void RebuildIcon();

  QString name_;
  QIcon icon_;
  QString icon_path_;
  QColor icon_color_;
  QString cache_path_;
  Util::SettingState enable_net_sec_;
  Util::SettingState persist_user_prefs_;
  QHash<QString, bool> browser_settings_;
};
\
}  // namespace doogie

#endif  // DOOGIE_BUBBLE_H_
