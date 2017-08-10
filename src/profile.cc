#include "profile.h"

#include <QtSql>

#include "action_manager.h"
#include "bubble.h"
#include "sql.h"
#include "util.h"
#include "workspace.h"

namespace doogie {

const QString Profile::kInMemoryPath = "<in-mem>";
const QString Profile::kAppDataPath = QDir::toNativeSeparators(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
Profile* Profile::current_ = nullptr;
QList<Profile::BrowserSetting> Profile::possible_browser_settings_ = {};

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
    if (set_current) return SetCurrent(new Profile(path, QJsonObject()));
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
  if (set_current) return SetCurrent(profile);
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
  if (set_current) return SetCurrent(profile);
  return true;
}

bool Profile::LoadProfileFromCommandLine(int argc, char* argv[]) {
  QString profile_path;
  bool allow_in_mem_fallback = true;

  // The profile can be specified via command line parameter --doogie-profile
  QCommandLineParser parser;
  parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
  // TODO(cretz): For now am I ok that there is no unicode?
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
  // We don't care if it's not successful
  parser.parse(args);
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
  if (possible_browser_settings_.isEmpty()) {
    possible_browser_settings_.append({
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
  return possible_browser_settings_;
}

QStringList Profile::LastTenProfilePaths() {
  return QSettings("cretz", "Doogie").value("profile/lastTen").toStringList();
}

Profile::Profile(const QString& path, const QJsonObject& obj)
    : path_(QDir::toNativeSeparators(path)) {
  cache_path_ = obj["cachePath"].toString();
  enable_net_sec_ = obj["enableNetSecurityExpiration"].toBool(false);
  // Default is true
  persist_user_prefs_ = obj["persistUserPreferences"].toBool(true);
  user_agent_ = obj["userAgent"].toString();
  user_data_path_ = obj["userDataPath"].toString();
  for (auto setting : PossibleBrowserSettings()) {
    if (obj.contains(setting.field)) {
      browser_settings_[setting.field] = obj[setting.field].toBool();
    }
  }
  QSet<QString> already_seen_names;
  for (auto bubble_json : obj["bubbles"].toArray()) {
    Bubble bubble(bubble_json.toObject());
    if (already_seen_names.contains(bubble.Name())) {
      qWarning() << "Duplicate bubble name: " << bubble.Name();
    } else {
      bubbles_.append(bubble);
    }
  }
  // If this is empty, we must have a single do-nothing bubble
  if (bubbles_.isEmpty()) {
    bubbles_.append(Bubble(QJsonObject()));
  }
  // Shortcuts can be key names or numbers
  auto shortcuts_obj = obj["shortcuts"].toObject();
  for (auto key : shortcuts_obj.keys()) {
    auto type = ActionManager::StringToType(key);
    if (type != -1) {
      QList<QKeySequence> seqs;
      for (auto val : shortcuts_obj[key].toArray()) {
        auto seq = Util::KeySequenceOrEmpty(val.toString());
        if (!seq.isEmpty()) seqs.append(seq);
      }
      shortcuts_[type] = seqs;
    }
  }
  // String list to longlong
  for (auto workspace_id : obj["openWorkspaceIds"].toArray()) {
    if (workspace_id.isString()) {
      auto ok = false;
      auto id = workspace_id.toString().toLongLong(&ok);
      if (ok) open_workspace_ids_.append(id);
    }
  }
}

void Profile::Init() {
  for (auto& bubble : bubbles_) {
    bubble.Init();
  }
}

void Profile::CopySettingsFrom(const Profile& profile) {
  cache_path_ = profile.cache_path_;
  enable_net_sec_ = profile.enable_net_sec_;
  persist_user_prefs_ = profile.persist_user_prefs_;
  user_agent_ = profile.user_agent_;
  user_data_path_ = profile.user_data_path_;
  browser_settings_ = profile.browser_settings_;
  bubbles_ = profile.bubbles_;
  shortcuts_ = profile.shortcuts_;
  open_workspace_ids_ = profile.open_workspace_ids_;
}

int Profile::BubbleIndexFromName(const QString& name) const {
  for (int i = 0; i < bubbles_.size(); i++) {
    if (bubbles_[i].Name() == name) return i;
  }
  return -1;
}

void Profile::ApplyCefSettings(CefSettings* settings) const {
  settings->no_sandbox = true;
  // Enable remote debugging on debug version
#ifdef QT_DEBUG
  settings->remote_debugging_port = 1989;
#endif

  QString cache_path = "";
  if (!InMemory()) {
    if (cache_path_.isNull()) {
      // Default is path/cache
      cache_path = QDir::toNativeSeparators(QDir(path_).filePath("cache"));
    } else if (!cache_path_.isEmpty()) {
      // Qualify it with the dir (but can still be absolute)
      cache_path = QDir::toNativeSeparators(
          QDir::cleanPath(QDir(path_).filePath(cache_path_)));
    }
  }
  CefString(&settings->cache_path) = cache_path.toStdString();

  if (enable_net_sec_) {
    settings->enable_net_security_expiration = 1;
  }

  if (persist_user_prefs_) {
    settings->persist_user_preferences = 1;
  }

  if (!user_agent_.isEmpty()) {
    CefString(&settings->user_agent) = user_agent_.toStdString();
  }

  // Default user data path to our own
  QString user_data_path = "";
  if (!InMemory()) {
    if (user_data_path_.isNull()) {
      user_data_path = QDir::toNativeSeparators(
            QDir(path_).filePath("user_data"));
    } else if (!user_data_path_.isEmpty()) {
      // Qualify it with the dir (but can still be absolute)
      user_data_path = QDir::toNativeSeparators(
            QDir::cleanPath(QDir(path_).filePath(user_data_path_)));
    }
  }
  CefString(&settings->user_data_path) = user_data_path.toStdString();
}

void Profile::ApplyCefBrowserSettings(CefBrowserSettings* settings) const {
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
  state(settings->javascript_dom_paste, "javascriptDomPaste");
  state(settings->javascript_open_windows, "javascriptOpenWindows");
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

void Profile::ApplyActionShortcuts() const {
  // Go over every action and set shortcut or default
  auto actions = ActionManager::Actions();
  for (auto type : actions.keys()) {
    actions[type]->setShortcuts(
        shortcuts_.value(type, ActionManager::DefaultShortcuts(type)));
  }
}

QJsonObject Profile::ToJson() const {
  QJsonObject ret;
  if (!cache_path_.isNull()) ret["cachePath"] = cache_path_;
  if (enable_net_sec_) ret["enableNetSecurityExpiration"] = true;
  if (!persist_user_prefs_) ret["persistUserPreferences"] = false;
  if (!user_agent_.isEmpty()) ret["userAgent"] = user_agent_;
  if (!user_data_path_.isNull()) ret["userDataPath"] = user_data_path_;
  for (auto setting_field : browser_settings_.keys()) {
    ret[setting_field] = browser_settings_[setting_field];
  }
  QJsonArray bubbles;
  for (const auto bubble : bubbles_) {
    bubbles.append(bubble.ToJson());
  }
  // If it's a single-bubble w/ an empty object, we don't add it
  if (bubbles.size() > 1 || !bubbles.first().toObject().isEmpty()) {
    ret["bubbles"] = bubbles;
  }
  QJsonObject shortcuts;
  for (auto type : shortcuts_.keys()) {
    QJsonArray seqs;
    for (auto seq : shortcuts_[type]) {
      seqs.append(seq.toString());
    }
    shortcuts[ActionManager::TypeToString(type)] = seqs;
  }
  if (!shortcuts.isEmpty()) ret["shortcuts"] = shortcuts;
  QJsonArray open_workspace_ids;
  for (auto id : open_workspace_ids_) {
    open_workspace_ids.append(QString::number(id));
  }
  ret["openWorkspaceIds"] = open_workspace_ids;
  return ret;
}

bool Profile::SavePrefs() const {
  // In memory saves nothing...
  if (InMemory()) return true;

  QFile file(QDir(path_).filePath("settings.doogie.json"));
  qDebug() << "Saving profile prefs to " << file.fileName();
  if (!file.open(QIODevice::WriteOnly)) {
    qCritical() << "Unable to open "<<
                   file.fileName() << " to save profile at";
    return false;
  }
  if (file.write(QJsonDocument(ToJson()).
                 toJson(QJsonDocument::Indented)) == -1) {\
    qCritical() << "Unable to write JSON to "<<
                   file.fileName() << " to save profile";
    return false;
  }
  return true;
}

bool Profile::RequiresRestartIfChangedTo(const Profile& other) const {
  return cache_path_ != other.cache_path_ ||
      cache_path_.isNull() != other.cache_path_.isNull() ||
      enable_net_sec_ != other.enable_net_sec_ ||
      persist_user_prefs_ != other.persist_user_prefs_ ||
      user_agent_ != other.user_agent_ ||
      user_data_path_ != other.user_data_path_ ||
      user_data_path_.isNull() != other.user_data_path_.isNull();
}

bool Profile::operator==(const Profile& other) const {
  return cache_path_ == other.cache_path_ &&
      cache_path_.isNull() == other.cache_path_.isNull() &&
      enable_net_sec_ == other.enable_net_sec_ &&
      persist_user_prefs_ == other.persist_user_prefs_ &&
      user_agent_ == other.user_agent_ &&
      user_data_path_ == other.user_data_path_ &&
      user_data_path_.isNull() == other.user_data_path_.isNull() &&
      browser_settings_ == other.browser_settings_ &&
      bubbles_ == other.bubbles_ &&
      shortcuts_ == other.shortcuts_ &&
      open_workspace_ids_ == other.open_workspace_ids_;
}

bool Profile::SetCurrent(Profile* profile) {
  if (current_) delete current_;
  current_ = profile;

  // We want to try to open a sqlite DB
  auto db = QSqlDatabase::addDatabase("QSQLITE");
  if (profile->InMemory()) {
    db.setDatabaseName(":memory:");
  } else {
    db.setDatabaseName(QDir::toNativeSeparators(
        QDir(profile->Path()).filePath("doogie.db")));
  }
  if (!db.open()) {
    qCritical() << "Unable to open doogie.db";
    return false;
  }
  if (!Sql::EnsureDatabaseSchema()) {
    qCritical() << "Unable to ensure schema is created";
    return false;
  }

  QSettings settings("cretz", "Doogie");
  settings.setValue("profile/lastLoaded", profile->path_);
  auto prev = settings.value("profile/lastTen").toStringList();
  prev.removeAll(profile->path_);
  prev.prepend(profile->path_);
  settings.setValue("profile/lastTen", prev);
  return true;
}

}  // namespace doogie
