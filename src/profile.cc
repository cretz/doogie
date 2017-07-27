#include "profile.h"
#include "bubble.h"

namespace doogie {

Profile* Profile::current_ = nullptr;

Profile* Profile::Current() {
  return current_;
}

bool Profile::CreateProfile(const QString& path) {
  // If the path is empty, it is a special case of mem-only
  if (path.isEmpty()) {
    SetCurrent(new Profile(path, QJsonObject()));
    return true;
  }
  // Make sure settings.doogie.json isn't already there
  if (QDir(path).exists("settings.doogie.json")) {
    return false;
  }
  // Attempt to create the path
  if (!QDir().mkpath(path)) {
    return false;
  }
  auto profile = new Profile(path, QJsonObject());
  // Try to save
  if (!profile->SavePrefs()) {
    return false;
  }
  SetCurrent(profile);
  return true;
}

bool Profile::LoadProfile(const QString& path) {
  if (!QDir(path).exists("settings.doogie.json")) {
    return false;
  }
  QFile file(QDir(path).filePath("settings.doogie.json"));\
  // Open for read only here, but we make sure we can save before continuing
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  auto json = QJsonDocument::fromJson(file.readAll());
  file.close();
  if (!json.isObject()) {
    return false;
  }
  // Create and try to save
  auto profile = new Profile(path, json.object());
  // Try to save
  if (!profile->SavePrefs()) {
    return false;
  }
  SetCurrent(profile);
  return true;
}

bool Profile::LoadProfileFromCommandLine(int argc, char* argv[]) {
  QString profile_path;
  bool allow_in_mem_fallback = true;

  // The profile can be specified via command line parameter --doogie-profile
  QCommandLineParser parser;
  // TODO: For now am I ok that there is no unicode?
  parser.addOption(QCommandLineOption("doogie-no-profile"));
  parser.addOption(QCommandLineOption("doogie-profile"));
  QStringList args;
  args.reserve(argc);
  for (int i = 0; i < argc; i++) {
    args.append(QString::fromLocal8Bit(argv[i]));
  }
  parser.process(args);
  if (parser.isSet("doogie-no-profile")) {
    // Do nothing
  } else if (parser.isSet("doogie-profile")) {
    profile_path = QDir::cleanPath(parser.value("doogie-profile"));
    allow_in_mem_fallback = false;
  } else {
    // We'll use the last loaded profile if we know of it
    QSettings settings;
    profile_path = settings.value("profile/lastLoaded").toString();
    // Make sure it's there
    if (!profile_path.isEmpty() &&
        !QDir(profile_path).exists("settings.doogie.json")) {
      qInfo() << "Last profile path " << profile_path <<
                 " no longer exists with settings, falling back to default";
      profile_path = "";
    }
    // Put at app_data/profiles/default by default
    if (profile_path.isEmpty()) {
      auto data_path = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
      if (data_path.isEmpty()) {
        qCritical() << "No app data path, only putting in memory";
      } else {
        profile_path = QDir::cleanPath(
              data_path + QDir::separator() +
              "Doogie" + QDir::separator() +
              "profiles" + QDir::separator() +
              "default");
      }
    }
  }

  // Try to load if the settings.doogie.json exists, otherwise
  // create
  bool success = false;
  if (!profile_path.isEmpty()) {
    QDir dir(profile_path);
    if (dir.exists("settings.doogie.json")) {
      success = LoadProfile(profile_path);
    } else {
      success = CreateProfile(profile_path);
    }
  }
  if (!success && allow_in_mem_fallback) {
    success = CreateProfile("");
  }
  return success;
}

CefSettings Profile::CreateCefSettings() {
  CefSettings settings;
  settings.no_sandbox = true;
  // Enable remote debugging on debug version
#ifdef QT_DEBUG
  settings.remote_debugging_port = 1989;
#endif

  QString cache_path;
  if (!prefs_.contains("cachePath")) {
    // Default is path/cache
    cache_path = QDir(path_).filePath("cache");
  } else {
    cache_path = prefs_["cachePath"].toString("");
    if (!cache_path.isEmpty()) {
      // Qualify it with the dir (but can still be absolute)
      cache_path = QDir::cleanPath(QDir(path_).filePath(cache_path));
    }
  }
  CefString(&settings.cache_path) = cache_path.toStdString();

  if (prefs_["enableNetSecurityExpiration"].toBool()) {
    settings.enable_net_security_expiration = 1;
  }

  // We default persist_user_preferences to true
  if (prefs_["persistUserPreferences"].toBool(true)) {
    settings.persist_user_preferences = 1;
  }

  if (prefs_.contains("userAgent")) {
    CefString(&settings.user_agent) =
        prefs_["userAgent"].toString().toStdString();
  }

  // Default user data path to our own
  QString user_data_path;
  if (!prefs_.contains("userDataPath")) {
    // Default is path/cache
    user_data_path = QDir(path_).filePath("user_data");
  } else {
    user_data_path = prefs_["userDataPath"].toString("");
    if (!user_data_path.isEmpty()) {
      // Qualify it with the dir (but can still be absolute)
      user_data_path = QDir::cleanPath(QDir(path_).filePath(user_data_path));
    }
  }
  CefString(&settings.user_data_path) = user_data_path.toStdString();

  return settings;
}

CefBrowserSettings Profile::CreateBrowserSettings() {
  CefBrowserSettings settings;

  auto browser = prefs_["browser"].toObject();

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

  return settings;
}

const QList<Bubble*> Profile::Bubbles() {
  return bubbles_;
}

Bubble* Profile::DefaultBubble() {
  return bubbles_.first();
}

Bubble* Profile::BubbleByName(const QString& name) {
  for (const auto& bubble : bubbles_) {
    if (bubble->Name() == name) {
      return bubble;
    }
  }
  return nullptr;
}

bool Profile::SavePrefs() {
  // Put all bubbles back
  QJsonArray arr;
  for (const auto& bubble : bubbles_) {
    arr.append(bubble->prefs_);
  }
  prefs_["bubbles"] = arr;

  QFile file(QDir(path_).filePath("settings.doogie.json"));
  qDebug() << "Saving profile prefs to " << file.fileName();
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }
  if (file.write(QJsonDocument(prefs_).
                 toJson(QJsonDocument::Indented)) == -1) {
    return false;
  }
  return true;
}

Profile::Profile(const QString& path, QJsonObject prefs, QObject* parent)
    : QObject(parent), path_(path), prefs_(prefs) {
  for (const auto& item : prefs["bubbles"].toArray()) {
    bubbles_.append(new Bubble(item.toObject(), this));
  }
  // If there are no bubbles, add an empty one
  if (bubbles_.isEmpty()) {
    bubbles_.append(new Bubble(QJsonObject(), this));
  }
}

void Profile::SetCurrent(Profile* profile) {
  if (current_) {
    delete current_;
  }
  qDebug() << "Setting current profile to " << profile->path_;
  current_ = profile;
  QSettings settings;
  if (!profile->path_.isEmpty()) {
    settings.setValue("profile/lastLoaded", profile->path_);
  }
}

}  // namespace doogie
