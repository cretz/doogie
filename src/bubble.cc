#include "bubble.h"
#include "util.h"

namespace doogie {

Bubble::~Bubble() {
}

QString Bubble::Name() {
  return prefs_.value("name").toString();
}

QString Bubble::FriendlyName() {
  auto ret = Name();
  if (!ret.isEmpty()) return ret;
  return "(default)";
}

void Bubble::ApplyCefBrowserSettings(CefBrowserSettings& settings) {
  Profile::Current()->ApplyCefBrowserSettings(settings);

  auto browser = prefs_.value("browser").toObject();

  auto state = [&browser](cef_state_t& state, const QString& field) {
    if (browser.contains(field)) {
      state = browser[field].toBool() ? STATE_ENABLED : STATE_DISABLED;
    }
  };

  state(settings.application_cache, "applicationCache");
  state(settings.databases, "databases");
  state(settings.file_access_from_file_urls, "fileAccessFromFileUrls");
  state(settings.image_loading, "imageLoading");
  state(settings.image_shrink_standalone_to_fit, "imageShrinkStandaloneToFit");
  state(settings.javascript, "javascript");
  state(settings.javascript_access_clipboard, "javascriptAccessClipboard");
  state(settings.javascript_dom_paste, "javvascriptDomPaste");
  state(settings.javascript_open_windows, "javascriptOpenWindows");
  state(settings.local_storage, "localStorage");
  state(settings.plugins, "plugins");
  state(settings.remote_fonts, "remoteFonts");
  state(settings.tab_to_links, "tabToLinks");
  state(settings.text_area_resize, "textAreaResize");
  state(settings.universal_access_from_file_urls,
        "universalAccessFromFileUrls");
  state(settings.web_security, "webSecurity");
  state(settings.webgl, "webgl");
}

void Bubble::ApplyCefRequestContextSettings(
    CefRequestContextSettings& settings) {
  CefSettings global;
  Profile::Current()->ApplyCefSettings(global);

  auto cef = prefs_.value("cef").toObject();

  if (cef.contains("cachePath")) {
    auto cache_path = cef["cachePath"].toString();
    // Empty/null is disabled, otherwise qualify w/ profile dir if necessary
    if (!cache_path.isEmpty()) {
      cache_path = QDir::toNativeSeparators(QDir::cleanPath(QDir(
          Profile::Current()->Path()).filePath(cache_path)));
    }
    CefString(&settings.cache_path) = cache_path.toStdString();
  } else {
    CefString(&settings.cache_path) = CefString(&global.cache_path).ToString();
  }

  if (cef.contains("enableNetSecurityExpiration")) {
    settings.enable_net_security_expiration =
        cef["enableNetSecurityExpiration"].toBool() ? 1 : 0;
  } else {
    settings.enable_net_security_expiration =
        global.enable_net_security_expiration;
  }

  if (cef.contains("persistUserPreferences")) {
    settings.persist_user_preferences =
        cef["persistUserPreferences"].toBool() ? 1 : 0;
  } else {
    settings.persist_user_preferences = global.persist_user_preferences;
  }

  // TODO: Due to a CEF bug, we cannot have user preferences persisted if we
  //  have a cache path. This is due to the fact that the pref JSON is created
  //  on the wrong thread.
  // See: https://bitbucket.org/chromiumembedded/cef/issues/2233
  if (settings.cache_path.length > 0 &&
      settings.persist_user_preferences == 1) {
    qCritical() << "Unable to have cache path and user prefs set "
                   "for a bubble due to internal bug.";
    settings.persist_user_preferences = 0;
  }
}

CefRefPtr<CefRequestContext> Bubble::CreateCefRequestContext() {
  CefRequestContextSettings settings;
  ApplyCefRequestContextSettings(settings);
  return CefRequestContext::CreateContext(settings, nullptr);
}

QIcon Bubble::Icon() {
  if (cached_icon_.isNull()) {
    if (prefs_.contains("icon")) {
      auto icon = prefs_["icon"].toString();
      if (icon.isEmpty()) icon = ":/res/images/fontawesome/circle.png";
      auto color = prefs_.value("color").toString();
      if (!QColor::isValidColor(color)) {
        cached_icon_ = Util::CachedIcon(icon);
      } else {
        cached_icon_ = QIcon(
              *Util::CachedPixmapColorOverlay(icon, QColor(color)));
      }
    } else {
      QPixmap pixmap(16, 16);
      pixmap.fill(Qt::transparent);
      cached_icon_ = QIcon(pixmap);
    }
  }
  return cached_icon_;
}

void Bubble::InvalidateIcon() {
  cached_icon_ = QIcon();
}

Bubble::Bubble(QJsonObject prefs, QObject* parent)
  : QObject(parent), prefs_(prefs) {

}

}  // namespace doogie
