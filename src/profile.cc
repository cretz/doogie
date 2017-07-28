#include "profile.h"
#include "bubble.h"
#include "util.h"

namespace doogie {

const QString Profile::kAppDataPath = QDir::toNativeSeparators(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
const QString Profile::kInMemoryPath = "<in-mem>";
Profile* Profile::current_ = nullptr;

Profile* Profile::Current() {
  return current_;
}

bool Profile::CreateProfile(const QString& path, bool set_current) {
  // If the path is in-mem (or empty), it is a special case of mem-only
  if (path.isEmpty() || path == kInMemoryPath) {
    if (set_current) SetCurrent(new Profile(path, QJsonObject()));
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
  auto profile = new Profile(QDir::toNativeSeparators(path), QJsonObject());
  // Try to save
  if (!profile->SavePrefs()) {
    return false;
  }
  if (set_current) SetCurrent(profile);
  return true;
}

bool Profile::LoadProfile(const QString& path, bool set_current) {
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
  auto profile = new Profile(QDir::toNativeSeparators(path), json.object());
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
    QSettings settings("cretz", "doogie");
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
    if (QDir(profile_path).exists("settings.doogie.json")) {
      success = LoadProfile(profile_path);
    } else {
      success = CreateProfile(profile_path);
    }
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

QString Profile::FriendlyName() {
  return FriendlyName(path_);
}

bool Profile::CanChangeSettings() {
  return path_ != kInMemoryPath;
}

CefSettings Profile::CreateCefSettings() {
  CefSettings settings;
  settings.no_sandbox = true;
  // Enable remote debugging on debug version
#ifdef QT_DEBUG
  settings.remote_debugging_port = 1989;
#endif

  QString cache_path;
  if (path_ == kInMemoryPath) {
    // Do nothing to cache path, leave it alone
  } else if (!prefs_.contains("cachePath")) {
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
  if (path_ == kInMemoryPath) {
    // Do nothing to user data path, leave it alone
  } else if (!prefs_.contains("userDataPath")) {
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
  // In memory saves nothing...
  if (!CanChangeSettings()) return true;
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

QString Profile::ShowChangeProfileDialog(bool& wants_restart) {
  auto layout = new QGridLayout;
  layout->addWidget(new QLabel("Current Profile:"), 0, 0);
  layout->addWidget(new QLabel(FriendlyName()), 0, 1);
  layout->addWidget(new QLabel("New/Existing Profile:"), 1, 0);
  auto selector = new QComboBox;
  selector->setEditable(true);
  selector->setInsertPolicy(QComboBox::NoInsert);
  QSettings settings("cretz", "doogie");
  // All but the current
  auto last_ten = settings.value("profile/lastTen").toStringList();
  auto found_in_mem = false;
  for (int i = 0; i < last_ten.size(); i++) {
    auto path = last_ten[i];
    if (path != path_) {
      if (path == kInMemoryPath) found_in_mem = true;
      selector->addItem(FriendlyName(path), path);
    }
  }
  if (!found_in_mem && path_ != kInMemoryPath) {
    selector->addItem(FriendlyName(kInMemoryPath), kInMemoryPath);
  }
  selector->addItem("<choose profile folder...>");
  selector->lineEdit()->setPlaceholderText("Get or Create Profile...");
  selector->setCurrentIndex(-1);
  layout->addWidget(selector, 1, 1);
  layout->setColumnStretch(1, 1);
  auto buttons = new QHBoxLayout;
  auto launch = new QPushButton("Launch New Doogie With Profile");
  launch->setEnabled(false);
  launch->setDefault(true);
  auto restart = new QPushButton("Restart Doogie With Profile");
  restart->setEnabled(false);
  auto cancel = new QPushButton("Cancel");
  buttons->addWidget(launch);
  buttons->addWidget(restart);
  buttons->addWidget(cancel);
  layout->addLayout(buttons, 2, 0, 1, 2, Qt::AlignRight);

  QDialog dialog;
  dialog.setWindowTitle("Change Profile");
  dialog.setLayout(layout);
  connect(selector, static_cast<void(QComboBox::*)(int)>(
            &QComboBox::currentIndexChanged), [this, selector](int index) {
    // If they clicked the last index, we pop open a file dialog
    if (index == selector->count() - 1) {
      auto open_dir = QDir(path_);
      open_dir.cdUp();
      auto path = QFileDialog::getExistingDirectory(
            nullptr, "Choose Profile Directory", open_dir.path());
      if (!path.isEmpty()) {
        selector->setCurrentText(FriendlyName(QDir::toNativeSeparators(path)));
      } else {
        selector->clearEditText();
        selector->setCurrentIndex(-1);
      }
    }
  });
  connect(selector, &QComboBox::editTextChanged,
          [this, restart, launch](const QString& text) {
    restart->setEnabled(!text.isEmpty());
    launch->setEnabled(!text.isEmpty());
  });
  connect(launch, &QPushButton::clicked,
          [this, &dialog]() { dialog.done(QDialog::Accepted + 1); });
  connect(restart, &QPushButton::clicked, &dialog, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
  auto result = dialog.exec();
  if (result == QDialog::Rejected) {
    return QString();
  }
  wants_restart = result == QDialog::Accepted;
  QString path;
  if (selector->currentData().isValid()) {
    path = selector->currentData().toString();
  } else {
    path = selector->currentText();
  }
  if (path != kInMemoryPath) {
    auto base_path = QDir(kAppDataPath).filePath("Doogie/profiles");
    path = QDir::toNativeSeparators(QDir(base_path).filePath(path));
    // Let's try to create the dir if it doesn't exist
    if (!QDir(path).exists("settings.doogie.json")) {
      if (!CreateProfile(path, false)) {
        QMessageBox::critical(
              nullptr, "Create Profile",
              QString("Unable to create profile at dir: ") + path);
        return QString();
      }
    } else if (!LoadProfile(path, false)) {
      QMessageBox::critical(
            nullptr, "Load Profile",
            QString("Unable to load profile in dir: ") + path);
      return QString();
    }
  }
  return path;
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
  current_ = profile;
  QSettings settings("cretz", "doogie");
  settings.setValue("profile/lastLoaded", profile->path_);
  auto prev = settings.value("profile/lastTen").toStringList();
  prev.removeAll(profile->path_);
  prev.prepend(profile->path_);
  settings.setValue("profile/lastTen", prev);
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

}  // namespace doogie
