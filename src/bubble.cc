#include "bubble.h"

#include "profile.h"
#include "util.h"

namespace doogie {

Bubble::Bubble(const QJsonObject& obj) {
  name_ = obj["name"].toString();
  icon_path_ = obj["iconPath"].toString();
  icon_color_ = QColor(obj["iconColor"].toString());
  cache_path_ = obj["cachePath"].toString();
  if (obj.contains("enableNetSecurityExpiration")) {
    enable_net_sec_ = obj["enableNetSecurityExpiration"].toBool() ?
          Util::Enabled : Util::Disabled;
  } else {
    enable_net_sec_ = Util::Default;
  }
  if (obj.contains("persistUserPreferences")) {
    persist_user_prefs_ = obj["persistUserPreferences"].toBool() ?
          Util::Enabled : Util::Disabled;
  } else {
    persist_user_prefs_ = Util::Default;
  }
  for (auto setting : Profile::PossibleBrowserSettings()) {
    if (obj.contains(setting.field)) {
      browser_settings_[setting.field] = obj[setting.field].toBool();
    }
  }
}

void Bubble::Init() {
  // This had to be deferred because of:
  //  "Must construct a QGuiApplication before a QPixmap"
  RebuildIcon();
}

void Bubble::CopySettingsFrom(const Bubble& other) {
  name_ = other.name_;
  // We move the icon too even though it's not a setting
  icon_ = other.icon_;
  icon_path_ = other.icon_path_;
  icon_color_ = other.icon_color_;
  cache_path_ = other.cache_path_;
  enable_net_sec_ = other.enable_net_sec_;
  persist_user_prefs_ = other.persist_user_prefs_;
  browser_settings_ = other.browser_settings_;
}

void Bubble::ApplyCefBrowserSettings(CefBrowserSettings* settings,
                                     bool include_current_profile) const {
  if (include_current_profile) {
    Profile::Current()->ApplyCefBrowserSettings(settings);
  }

  auto state = [=](cef_state_t& state, const QString& field) {
    if (browser_settings_.contains(field)) {
      state = browser_settings_[field] ? STATE_ENABLED : STATE_DISABLED;
    }
  };

  state(settings->application_cache, "applicationCache");
  state(settings->databases, "databases");
  state(settings->file_access_from_file_urls, "fileAccessFromFileUrls");
  state(settings->image_loading, "imageLoading");
  state(settings->image_shrink_standalone_to_fit, "imageShrinkStandaloneToFit");
  state(settings->javascript, "javascript");
  state(settings->javascript_access_clipboard, "javascriptAccessClipboard");
  state(settings->javascript_dom_paste, "javvascriptDomPaste");
  state(settings->local_storage, "localStorage");
  state(settings->plugins, "plugins");
  state(settings->remote_fonts, "remoteFonts");
  state(settings->tab_to_links, "tabToLinks");
  state(settings->text_area_resize, "textAreaResize");
  state(settings->universal_access_from_file_urls,
        "universalAccessFromFileUrls");
  state(settings->web_security, "webSecurity");
  state(settings->webgl, "webgl");
}

void Bubble::ApplyCefRequestContextSettings(
    CefRequestContextSettings* settings,
    bool include_current_profile) const {
  CefSettings global;
  if (include_current_profile) {
    Profile::Current()->ApplyCefSettings(&global);
  }

  if (cache_path_.isNull()) {
    CefString(&settings->cache_path) =
        CefString(&global.cache_path).ToString();
  } else {
    // Empty/null is disabled, otherwise qualify w/ profile dir if necessary
    if (!cache_path_.isEmpty()) {
      CefString(&settings->cache_path) =
          QDir::toNativeSeparators(QDir::cleanPath(QDir(
              Profile::Current()->Path()).filePath(cache_path_))).toStdString();
    } else {
      CefString(&settings->cache_path) = cache_path_.toStdString();
    }
  }

  if (enable_net_sec_ == Util::Default) {
    settings->enable_net_security_expiration =
        global.enable_net_security_expiration;
  } else {
    settings->enable_net_security_expiration =
        (enable_net_sec_ == Util::Enabled) ? 1 : 0;
  }

  if (persist_user_prefs_ == Util::Default) {
    settings->persist_user_preferences =
        global.persist_user_preferences;
  } else {
    settings->persist_user_preferences =
        (persist_user_prefs_ == Util::Enabled) ? 1 : 0;
  }

  // TODO(cretz): Due to a CEF bug, we cannot have user preferences persisted
  //  if we have a cache path. This is due to the fact that the pref JSON is
  //  created on the wrong thread.
  // See: https://bitbucket.org/chromiumembedded/cef/issues/2233
  if (settings->cache_path.length > 0 &&
      settings->persist_user_preferences == 1) {
    qWarning() <<
      "Unable to have cache path and user pref persistence set "
      "for a bubble due to internal bug. User pref persistence "
      "will be disabled";
    settings->persist_user_preferences = 0;
  }
}

CefRefPtr<CefRequestContext> Bubble::CreateCefRequestContext() const {
  CefRequestContextSettings settings;
  ApplyCefRequestContextSettings(&settings);
  return CefRequestContext::CreateContext(settings, nullptr);
}

QJsonObject Bubble::ToJson() const {
  QJsonObject obj;
  if (!name_.isEmpty()) obj["name"] = name_;
  if (!icon_path_.isNull()) obj["iconPath"] = icon_path_;
  if (icon_color_.isValid()) obj["iconColor"] = icon_color_.name();
  if (!cache_path_.isNull()) obj["cachePath"] = cache_path_;
  if (enable_net_sec_ != Util::Default) {
    obj["enableNetSecurityExpiration"] = enable_net_sec_ == Util::Enabled;
  }
  if (persist_user_prefs_ != Util::Default) {
    obj["persistUserPreferences"] = persist_user_prefs_ == Util::Enabled;
  }
  for (auto setting_field : browser_settings_.keys()) {
    obj[setting_field] = browser_settings_[setting_field];
  }
  return obj;
}

bool Bubble::operator==(const Bubble& other) const {
  return name_ == other.name_ &&
      icon_path_ == other.icon_path_ &&
      icon_path_.isNull() == other.icon_path_.isNull() &&
      icon_color_ == other.icon_color_ &&
      cache_path_ == other.cache_path_ &&
      cache_path_.isNull() == other.cache_path_.isNull() &&
      enable_net_sec_ == other.enable_net_sec_ &&
      persist_user_prefs_ == other.persist_user_prefs_ &&
      browser_settings_ == other.browser_settings_;
}

void Bubble::RebuildIcon() {
  if (icon_path_.isNull()) {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    icon_ = QIcon(pixmap);
  } else {
    auto path = icon_path_;
    if (path.isEmpty()) path = ":/res/images/fontawesome/circle.png";
    if (icon_color_.isValid()) {
      icon_ = QIcon(*Util::CachedPixmapColorOverlay(path, QColor(icon_color_)));
    } else {
      icon_ = Util::CachedIcon(path);
    }
  }
}

}  // namespace doogie
