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
Profile Profile::current_(kInMemoryPath);

bool Profile::CreateOrLoadProfile(const QString& path, bool set_current) {
  if (path == kInMemoryPath) return CreateProfile(path, set_current);
  auto abs_path = QDir::toNativeSeparators(
        QDir(QDir(kAppDataPath).filePath("Doogie/profiles")).filePath(path));
  if (!QDir(abs_path).exists("doogie.db")) {
    return CreateProfile(abs_path, set_current);
  }
  return LoadProfile(abs_path, set_current);
}

bool Profile::CreateProfile(const QString& path, bool set_current) {
  auto abs_path = kInMemoryPath;
  // If the path is in-mem (or empty), it is a special case of mem-only
  if (!path.isEmpty() && path != kInMemoryPath) {
    abs_path = QDir::toNativeSeparators(
          QDir(QDir(kAppDataPath).filePath("Doogie/profiles")).filePath(path));
    // Make sure DB isn't already there
    if (QDir(abs_path).exists("doogie.db")) return false;
    // Attempt to create the path
    if (!QDir().mkpath(abs_path)) return false;
  }
  Profile profile(abs_path);
  profile.ApplyDefaults();
  if (set_current) {
    if (!SetCurrent(profile)) return false;
    // Save the current ref here
    return Profile::Current().Persist();
  }
  return profile.Persist();
}

bool Profile::LoadProfile(const QString& path, bool set_current) {
  auto abs_path = QDir::toNativeSeparators(
        QDir(QDir(kAppDataPath).filePath("Doogie/profiles")).filePath(path));
  if (!QDir(abs_path).exists("doogie.db")) return false;
  // Create and try to load if setting current
  Profile profile(abs_path);
  if (set_current) {
    if (!SetCurrent(profile)) return false;
    // Reload the current ref here
    return Profile::Current().Reload();
  }
  return profile.Reload();
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
      if (!profile_path.isEmpty() && !QDir(profile_path).exists("doogie.db")) {
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

  // Try to load if exists, otherwise create
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

QStringList Profile::LastTenProfilePaths() {
  return QSettings("cretz", "Doogie").value("profile/lastTen").toStringList();
}

bool Profile::Persist() {
  // Just delete and re-insert
  QSqlQuery query;
  if (!Sql::Exec(&query, "DELETE FROM profile")) return false;
  if (!Sql::Exec(&query, "DELETE FROM action_shortcut")) return false;

  auto ok = Sql::ExecParam(
        &query,
        "INSERT INTO profile ( "
        "  cache_path, user_agent, user_data_path "
        ") VALUES (?, ?, ?)",
        { cache_path_, user_agent_, user_data_path_ });
  if (!ok) return false;
  if (!shortcuts_.isEmpty()) {
    QStringList values;
    for (auto key : shortcuts_.keys()) {
      values << QString("('%1', '%2')").
                arg(ActionManager::TypeToString(key)).
                arg(QKeySequence::listToString(shortcuts_[key]).
                    replace('\'', "''"));
    }
    ok = Sql::Exec(&query,
                   QString("INSERT INTO action_shortcut "
                           "(key, shortcuts) VALUES %1").
                   arg(values.join(',')));
    if (!ok) return false;
  }
  return Bubble::PersistBrowserSettings(browser_settings_) &&
      Bubble::PersistEnabledBlockerListIds(enabled_blocker_list_ids_);
}

bool Profile::Reload() {
  QSqlQuery query;
  auto record = Sql::ExecSingle(&query, "SELECT * FROM profile");
  if (record.isEmpty()) return false;
  cache_path_ = record.value("cache_path").toString();
  user_agent_ = record.value("user_agent").toString();
  user_data_path_ = record.value("user_data_path").toString();
  shortcuts_.clear();
  if (!Sql::Exec(&query, "SELECT * FROM action_shortcut")) return false;
  while (query.next()) {
    shortcuts_[ActionManager::StringToType(query.value("key").toString())] =
        QKeySequence::listFromString(query.value("shortcuts").toString());
  }
  browser_settings_ = Bubble::LoadBrowserSettings();
  enabled_blocker_list_ids_ = Bubble::LoadEnabledBlockerListIds();
  return true;
}

void Profile::CopySettingsFrom(const Profile& other) {
  path_ = other.path_;
  user_agent_ = other.user_agent_;
  user_data_path_ = other.user_data_path_;
  browser_settings_ = other.browser_settings_;
  shortcuts_ = other.shortcuts_;
  enabled_blocker_list_ids_ = other.enabled_blocker_list_ids_;
}

void Profile::ApplyCefSettings(CefSettings* settings) const {
  Bubble::ApplyBrowserSettings(browser_settings_, settings);
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
  Bubble::ApplyBrowserSettings(browser_settings_, settings);
}

void Profile::ApplyActionShortcuts() const {
  // Go over every action and set shortcut or default
  auto actions = ActionManager::Actions();
  for (auto type : actions.keys()) {
    actions[type]->setShortcuts(
        shortcuts_.value(type, ActionManager::DefaultShortcuts(type)));
  }
}

bool Profile::RequiresRestartIfChangedTo(const Profile& other) const {
  return cache_path_ != other.cache_path_ ||
      cache_path_.isNull() != other.cache_path_.isNull() ||
      browser_settings_.value(BrowserSetting::EnableNetSecurityExpiration) !=
        other.browser_settings_.value(
          BrowserSetting::EnableNetSecurityExpiration) ||
      browser_settings_.value(BrowserSetting::PersistUserPreferences) !=
        other.browser_settings_.value(
          BrowserSetting::PersistUserPreferences) ||
      user_agent_ != other.user_agent_ ||
      user_data_path_ != other.user_data_path_ ||
      user_data_path_.isNull() != other.user_data_path_.isNull();
}

bool Profile::operator==(const Profile& other) const {
  return cache_path_ == other.cache_path_ &&
      cache_path_.isNull() == other.cache_path_.isNull() &&
      user_agent_ == other.user_agent_ &&
      user_data_path_ == other.user_data_path_ &&
      user_data_path_.isNull() == other.user_data_path_.isNull() &&
      browser_settings_ == other.browser_settings_ &&
      shortcuts_ == other.shortcuts_ &&
      enabled_blocker_list_ids_ == other.enabled_blocker_list_ids_;
}

bool Profile::SetCurrent(const Profile& profile) {
  current_ = profile;

  // We want to try to open a sqlite DB
  auto db = QSqlDatabase::addDatabase("QSQLITE");
  if (current_.InMemory()) {
    db.setDatabaseName(":memory:");
  } else {
    db.setDatabaseName(QDir::toNativeSeparators(
        QDir(current_.path_).filePath("doogie.db")));
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
  settings.setValue("profile/lastLoaded", current_.path_);
  auto prev = settings.value("profile/lastTen").toStringList();
  prev.removeAll(current_.path_);
  prev.prepend(current_.path_);
  settings.setValue("profile/lastTen", prev);
  return true;
}

Profile::Profile(const QString& path) {
  path_ = path;
  if (path_.isEmpty()) path_ = kInMemoryPath;
}

void Profile::ApplyDefaults() {
  // Only user prefs for now
  browser_settings_[BrowserSetting::PersistUserPreferences] = true;
}

}  // namespace doogie
