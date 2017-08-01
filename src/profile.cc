#include "profile.h"
#include "bubble.h"
#include "util.h"
#include "action_manager.h"

namespace doogie {

const QString Profile::kInMemoryPath = "<in-mem>";
Profile* Profile::current_ = nullptr;
const QString Profile::kAppDataPath = QDir::toNativeSeparators(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
QList<Profile::BrowserSetting> Profile::browser_settings_ = {};

Profile* Profile::Current() {
  return current_;
}

bool Profile::CreateOrLoadProfile(const QString& path, bool set_current) {
  if (path == kInMemoryPath) return CreateProfile(path, set_current);
  auto abs_path = QDir::toNativeSeparators(
        QDir(QDir(kAppDataPath).filePath("Doogie/profiles")).filePath(path));
  if (!QDir(abs_path).exists("settings.doogie.json")) {
    return CreateProfile(abs_path, set_current);
  }
  return LoadProfile(abs_path, set_current);
}

bool Profile::CreateProfile(const QString& path, bool set_current) {
  // If the path is in-mem (or empty), it is a special case of mem-only
  if (path.isEmpty() || path == kInMemoryPath) {
    if (set_current) SetCurrent(new Profile(path, QJsonObject()));
    return true;
  }
  auto abs_path = QDir::toNativeSeparators(
        QDir(QDir(kAppDataPath).filePath("Doogie/profiles")).filePath(path));
  // Make sure settings.doogie.json isn't already there
  if (QDir(abs_path).exists("settings.doogie.json")) {
    return false;
  }
  // Attempt to create the path
  if (!QDir().mkpath(abs_path)) {
    return false;
  }
  auto profile = new Profile(abs_path, QJsonObject());
  // Try to save
  if (!profile->SavePrefs()) {
    return false;
  }
  if (set_current) SetCurrent(profile);
  return true;
}

bool Profile::LoadProfile(const QString& path, bool set_current) {
  auto abs_path = QDir::toNativeSeparators(
        QDir(QDir(kAppDataPath).filePath("Doogie/profiles")).filePath(path));
  if (!QDir(abs_path).exists("settings.doogie.json")) {
    return false;
  }
  QFile file(QDir(abs_path).filePath("settings.doogie.json"));\
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
  auto profile = new Profile(abs_path, json.object());
  // Try to save
  if (!profile->SavePrefs()) {
    return false;
  }
  if (set_current) SetCurrent(profile);
  return true;
}

bool Profile::LoadProfileFromCommandLine(int argc, char* argv[]) {
  QString profile_path;
  bool allow_in_mem_fallback = true;

  // The profile can be specified via command line parameter --doogie-profile
  QCommandLineParser parser;
  parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
  // TODO: For now am I ok that there is no unicode?
  QCommandLineOption noProfileOption("doogie-no-profile");
  parser.addOption(noProfileOption);
  QCommandLineOption profileOption("doogie-profile");
  profileOption.setValueName("profilePath");
  parser.addOption(profileOption);
  QStringList args;
  args.reserve(argc);
  for (int i = 0; i < argc; i++) {
    args.append(QString::fromLocal8Bit(argv[i]));
  }
  parser.process(args);
  if (parser.isSet(noProfileOption)) {
    profile_path = kInMemoryPath;
  } else if (parser.isSet(profileOption)) {
    profile_path = QDir::cleanPath(parser.value(profileOption));
    allow_in_mem_fallback = false;
  } else {
    // We'll use the last loaded profile if we know of it
    QSettings settings("cretz", "Doogie");
    profile_path = settings.value("profile/lastLoaded").toString();
    // If in-memory was last, it's ok
    if (profile_path != kInMemoryPath) {
      // Make sure it's there
      if (!profile_path.isEmpty() &&
          !QDir(profile_path).exists("settings.doogie.json")) {
        qInfo() << "Last profile path " << profile_path <<
                   " no longer exists with settings, falling back to default";
        profile_path = "";
      }
      // Put at app_data/profiles/default by default
      if (profile_path.isEmpty()) {
        if (kAppDataPath.isEmpty()) {
          qCritical() << "No app data path, only putting in memory";
        } else {
          profile_path = QDir::cleanPath(
                kAppDataPath + QDir::separator() +
                "Doogie" + QDir::separator() +
                "profiles" + QDir::separator() +
                "default");
        }
      }
    }
  }

  // Try to load if the settings.doogie.json exists, otherwise
  // create
  bool success = false;
  qDebug() << "Loading profile: " << profile_path;
  if (!profile_path.isEmpty() && profile_path != kInMemoryPath) {
    success = CreateOrLoadProfile(profile_path);
    if (!success) {
      qCritical() << "Failed to create/load profile: " << profile_path;
    }
  }
  if (!success && (allow_in_mem_fallback || profile_path == kInMemoryPath)) {
    success = CreateProfile(kInMemoryPath);
  }
  return success;
}

bool Profile::LaunchWithProfile(const QString& profile_path) {
  // Build args with existing profile stuff removed
  auto args = QCoreApplication::arguments();
  auto prog = args.takeFirst();
  args.removeAll("-doogie-no-profile");
  args.removeAll("--doogie-no-profile");
  auto prev_arg_index = args.indexOf("--doogie-profile");
  if (prev_arg_index != -1) {
    args.removeAt(prev_arg_index);
    if (prev_arg_index < args.size()) args.removeAt(prev_arg_index);
  }
  prev_arg_index = args.indexOf("-doogie-profile");
  if (prev_arg_index != -1) {
    args.removeAt(prev_arg_index);
    if (prev_arg_index < args.size()) args.removeAt(prev_arg_index);
  }
  if (profile_path == kInMemoryPath) {
    args.append("--doogie-no-profile");
  } else {
    args.append("--doogie-profile");
    args.append(profile_path);
  }
  qDebug() << "Starting " << prog << args.join(' ');
  auto success = QProcess::startDetached(prog, args);
  if (!success) {
    QMessageBox::critical(nullptr, "Launch failed",
                          QString("Unable to launch Doogie with profile at ") +
                          profile_path);
  }
  return success;
}

QString Profile::FriendlyName(const QString& path) {
  if (path == kInMemoryPath) return "In-Memory Profile";
  // Ug, QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
  //  returns different values whether app is started or not, so we made
  //  it a const
  // Basically just chop off the app data path if it's there
  auto ret = path;
  if (!kAppDataPath.isEmpty()) {
    auto data_path = kAppDataPath + QDir::separator() +
        "Doogie" + QDir::separator() +
        "profiles" + QDir::separator();
    if (ret.startsWith(data_path, Qt::CaseInsensitive)) {
      ret = ret.mid(data_path.length());
    }
  }
  return ret;
}

QList<Profile::BrowserSetting> Profile::PossibleBrowserSettings() {
  // We know this is not thread safe; fine w/ us
  if (browser_settings_.isEmpty()) {
    browser_settings_.append({
      Profile::BrowserSetting {
        "Application Cache?",
        "Controls whether the application cache can be used.",
        "applicationCache" },
      Profile::BrowserSetting {
        "Databases?",
        "Controls whether databases can be used.",
        "databases" },
      Profile::BrowserSetting {
        "File Access From File URLs?",
        "Controls whether file URLs will have access to other file URLs.",
        "fileAccessFromFileUrls" },
      Profile::BrowserSetting {
        "Image Loading?",
        "Controls whether image URLs will be loaded from the network.",
        "imageLoading" },
      Profile::BrowserSetting {
        "Image Fit To Page?",
        "Controls whether standalone images will be shrunk to fit the page.",
        "imageShrinkStandaloneToFit" },
      Profile::BrowserSetting {
        "JavaScript?",
        "Controls whether JavaScript can be executed.",
        "javascript" },
      Profile::BrowserSetting {
        "JavaScript Clipboard Access?",
        "Controls whether JavaScript can access the clipboard.",
        "javascriptAccessClipboard" },
      Profile::BrowserSetting {
        "JavaScript DOM Paste?",
        "Controls whether DOM pasting is supported in the editor via "
        "execCommand(\"paste\"). Requires JavaScript clipboaard access.",
        "javascriptDomPaste" },
      Profile::BrowserSetting {
        "JavaScript Open Windows?",
        "Controls whether JavaScript can be used for opening windows.",
        "javascriptOpenWindows" },
      Profile::BrowserSetting {
        "Local Storage?",
        "Controls whether local storage can be used.",
        "localStorage" },
      Profile::BrowserSetting {
        "Browser Plugins?",
        "Controls whether any plugins will be loaded.",
        "plugins" },
      Profile::BrowserSetting {
        "Remote Fonts?",
        "Controls the loading of fonts from remote sources.",
        "remoteFonts" },
      Profile::BrowserSetting {
        "Tabs to Links?",
        "Controls whether the tab key can advance focus to links.",
        "tabToLinks" },
      Profile::BrowserSetting {
        "Text Area Resize?",
        "Controls whether text areas can be resized.",
        "textAreaResize" },
      Profile::BrowserSetting {
        "Universal Access From File URLs?",
        "Controls whether file URLs will have access to all URLs.",
        "universalAccessFromFileUrls" },
      Profile::BrowserSetting {
        "Web Security?",
        "Controls whether web security restrictions (same-origin policy) "
        "will be enforced.",
        "webSecurity" },
      Profile::BrowserSetting {
        "WebGL?",
        "Controls whether WebGL can be used.",
        "webgl" }
    });
  }
  return browser_settings_;
}

QStringList Profile::LastTenProfilePaths() {
  return QSettings("cretz", "Doogie").value("profile/lastTen").toStringList();
}

QKeySequence Profile::KeySequenceOrEmpty(const QString& str) {
  if (str.isEmpty()) return QKeySequence();
  auto seq = QKeySequence::fromString(str);
  if (seq.isEmpty()) return QKeySequence();
  for (int i = 0; i < seq.count(); i++) {
    if (seq[i] == Qt::Key_unknown) return QKeySequence();
  }
  return seq;
}

QString Profile::FriendlyName() {
  return FriendlyName(path_);
}

QString Profile::Path() {
  return path_;
}

bool Profile::InMemory() {
  return path_ == kInMemoryPath;
}

CefSettings Profile::CreateCefSettings() {
  CefSettings settings;
  settings.no_sandbox = true;
  // Enable remote debugging on debug version
#ifdef QT_DEBUG
  settings.remote_debugging_port = 1989;
#endif

  auto cef = prefs_.value("cef").toObject();

  QString cache_path;
  if (path_ == kInMemoryPath) {
    // Do nothing to cache path, leave it alone
  } else if (!cef.contains("cachePath")) {
    // Default is path/cache
    cache_path = QDir(path_).filePath("cache");
  } else {
    cache_path = cef.value("cachePath").toString("");
    if (!cache_path.isEmpty()) {
      // Qualify it with the dir (but can still be absolute)
      cache_path = QDir::cleanPath(QDir(path_).filePath(cache_path));
    }
  }
  CefString(&settings.cache_path) = cache_path.toStdString();

  if (cef.value("enableNetSecurityExpiration").toBool()) {
    settings.enable_net_security_expiration = 1;
  }

  // We default persist_user_preferences to true
  if (cef.value("persistUserPreferences").toBool(true)) {
    settings.persist_user_preferences = 1;
  }

  if (cef.contains("userAgent")) {
    CefString(&settings.user_agent) =
        prefs_.value("userAgent").toString().toStdString();
  }

  // Default user data path to our own
  QString user_data_path;
  if (path_ == kInMemoryPath) {
    // Do nothing to user data path, leave it alone
  } else if (!cef.contains("userDataPath")) {
    // Default is path/cache
    user_data_path = QDir(path_).filePath("user_data");
  } else {
    user_data_path = cef.value("userDataPath").toString("");
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

  auto browser = prefs_.value("browser").toObject();

  auto state = [&browser](cef_state_t& state, const QString& field) {
    if (browser.contains(field)) {
      state = browser.value(field).toBool() ? STATE_ENABLED : STATE_DISABLED;
    }
  };

  state(settings.application_cache, "applicationCache");
  state(settings.databases, "databases");
  state(settings.file_access_from_file_urls, "fileAccessFromFileUrls");
  state(settings.image_loading, "imageLoading");
  state(settings.image_shrink_standalone_to_fit, "imageShrinkStandaloneToFit");
  state(settings.javascript, "javascript");
  state(settings.javascript_access_clipboard, "javascriptAccessClipboard");
  state(settings.javascript_dom_paste, "javascriptDomPaste");
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
  // In memory saves nothing...
  if (InMemory()) return true;
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

void Profile::ApplyActionShortcuts() {
  // Go over all actions, do what the prefs or the default if
  // no prefs
  auto action_meta = QMetaEnum::fromType<ActionManager::Type>();
  auto prefs = prefs_.value("shortcuts").toObject();
  for (int i = 0; i < action_meta.keyCount(); i++) {
    auto action = ActionManager::Action(action_meta.value(i));
    if (action) {
      auto seqs = ActionManager::DefaultShortcuts(action_meta.value(i));
      if (prefs.contains(action_meta.key(i))) {
        seqs.clear();
        for (auto& pref : prefs[action_meta.key(i)].toArray()) {
          auto seq = KeySequenceOrEmpty(pref.toString());
          if (!seq.isEmpty()) seqs.append(seq);
        }
      }
      action->setShortcuts(seqs);
    }
  }
}

Profile::Profile(const QString& path, QJsonObject prefs, QObject* parent)
    : QObject(parent), path_(path), prefs_(prefs) {
  for (const auto& item : prefs.value("bubbles").toArray()) {
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
  current_ = profile;
  QSettings settings("cretz", "Doogie");
  settings.setValue("profile/lastLoaded", profile->path_);
  auto prev = settings.value("profile/lastTen").toStringList();
  prev.removeAll(profile->path_);
  prev.prepend(profile->path_);
  settings.setValue("profile/lastTen", prev);
}

}  // namespace doogie
