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
  QSettings settings("cretz", "Doogie");
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

QString Profile::ShowProfileSettingsDialog(bool& wants_restart) {
  // Pages:
  // * Browser Settings (any changes require restart)
  // * Bubbles
  // * Keyboard Shortcuts

  // Page: Browser Settings
  auto cef = prefs_.value("cef").toObject();

  auto browser_layout = new QGridLayout;
  browser_layout->setVerticalSpacing(3);
  auto warn_restart =
      new QLabel("NOTE: Changing browser settings requires a restart");
  warn_restart->setStyleSheet("color: red; font-weight: bold");
  browser_layout->addWidget(warn_restart, 0, 0, 1, 2);
  auto create_setting = [browser_layout](
      const QString& name, const QString& desc, QWidget* widg) {
    auto name_label = new QLabel(name);
    name_label->setStyleSheet("font-weight: bold;");
    auto row = browser_layout->rowCount();
    browser_layout->addWidget(name_label, row, 0);
    auto desc_label = new QLabel(desc);
    desc_label->setWordWrap(true);
    browser_layout->addWidget(widg, row, 1);
    browser_layout->addWidget(desc_label, row + 1, 0, 1, 2);
  };
  auto create_bool_setting = [create_setting](
      const QString& name,
      const QString& desc,
      bool curr,
      bool default_val) -> QComboBox* {
    auto widg = new QComboBox;
    widg->addItem(QString("Yes") + (default_val ? " (default)" : ""), true);
    widg->addItem(QString("No") + (!default_val ? " (default)" : ""), false);
    widg->setCurrentIndex(curr ? 0 : 1);
    create_setting(name, desc, widg);
    return widg;
  };
  auto setting_break = [browser_layout]() {
    auto frame = new QFrame;
    frame->setFrameShape(QFrame::HLine);
    browser_layout->addWidget(frame, browser_layout->rowCount(), 0, 1, 2);
  };

  auto cache_path_layout = new QHBoxLayout;
  auto cache_path_disabled = new QCheckBox("No cache");
  cache_path_layout->addWidget(cache_path_disabled);
  auto cache_path_edit = new QLineEdit;
  cache_path_edit->setPlaceholderText("Default: PROFILE_DIR/cache");
  cache_path_layout->addWidget(cache_path_edit, 1);
  auto cache_path_open = new QToolButton();
  cache_path_open->setText("...");
  cache_path_layout->addWidget(cache_path_open);
  auto cache_path_widg = new QWidget;
  cache_path_widg->setLayout(cache_path_layout);
  create_setting("Cache Path",
                 "The location where cache data is stored on disk, if any. ",
                 cache_path_widg);
  connect(cache_path_disabled, &QCheckBox::toggled,
          [this, cache_path_edit, cache_path_open](bool checked) {
    cache_path_edit->setEnabled(!checked);
    cache_path_open->setEnabled(!checked);
  });
  auto curr_cache_path = cef.contains("cachePath") ?
        cef.value("cachePath").toString() : QString();
  cache_path_disabled->setChecked(!curr_cache_path.isNull() &&
                                  curr_cache_path.isEmpty());
  cache_path_edit->setText(curr_cache_path);
  setting_break();

  auto enable_net_sec = create_bool_setting(
        "Enable Net Security Expiration",
        "Enable date-based expiration of built in network security information "
        "(i.e. certificate transparency logs, HSTS preloading and pinning "
        "information). Enabling this option improves network security but may "
        "cause HTTPS load failures when using CEF binaries built more than 10 "
        "weeks in the past. See https://www.certificate-transparency.org/ and "
        "https://www.chromium.org/hsts for details.",
        cef.value("enableNetSecurityExpiration").toBool(),
        false);
  setting_break();

  auto user_prefs = create_bool_setting(
        "Persist User Preferences",
        "Whether to persist user preferences as a JSON file "
        "in the cache path. Requires cache to be enabled.",
        cef.value("persistUserPreferences").toBool(true),
        true);
  setting_break();

  auto user_agent_edit = new QLineEdit;
  user_agent_edit->setPlaceholderText("Browser default");
  create_setting("User Agent",
                 "Custom user agent override.",
                 user_agent_edit);
  if (cef.contains("userAgent")) {
    user_agent_edit->setText(cef.value("userAgent").toString());
  }
  setting_break();

  auto user_data_path_layout = new QHBoxLayout;
  auto user_data_path_disabled = new QCheckBox("Browser default");
  user_data_path_layout->addWidget(user_data_path_disabled);
  auto user_data_path_edit = new QLineEdit;
  user_data_path_edit->setPlaceholderText("Default: PROFILE_DIR/user_data");
  user_data_path_layout->addWidget(user_data_path_edit, 1);
  auto user_data_path_open = new QToolButton();
  user_data_path_open->setText("...");
  user_data_path_layout->addWidget(user_data_path_open);
  auto user_data_path_widg = new QWidget;
  user_data_path_widg->setLayout(user_data_path_layout);
  create_setting("User Data Path",
                 "The location where user data such as spell checking "
                 "dictionary files will be stored on disk.",
                 user_data_path_widg);
  connect(user_data_path_disabled, &QCheckBox::toggled,
          [this, user_data_path_edit, user_data_path_open](bool checked) {
    user_data_path_edit->setEnabled(!checked);
    user_data_path_open->setEnabled(!checked);
  });
  auto curr_user_data_path = cef.contains("userDataPath") ?
        cef.value("userDataPath").toString() : QString();
  user_data_path_disabled->setChecked(!curr_user_data_path.isNull() &&
                                      curr_user_data_path.isEmpty());
  user_data_path_edit->setText(curr_user_data_path);
  setting_break();

  auto browser = prefs_.value("browser").toObject();
  // <name, desc, field>
  typedef std::tuple<QString, QString, QString> setting;
  QList<setting> settings({
      std::make_tuple(
          "Application Cache?",
          "Controls whether the application cache can be used.",
          "applicationCache"),
      std::make_tuple(
          "Databases?",
          "Controls whether databases can be used.",
          "databases"),
      std::make_tuple(
          "File Access From File URLs?",
          "Controls whether file URLs will have access to other file URLs.",
          "fileAccessFromFileUrls"),
      std::make_tuple(
          "Image Loading?",
          "Controls whether image URLs will be loaded from the network.",
          "imageLoading"),
      std::make_tuple(
          "Image Fit To Page?",
          "Controls whether standalone images will be shrunk to fit the page.",
          "imageShrinkStandaloneToFit"),
      std::make_tuple(
          "JavaScript?",
          "Controls whether JavaScript can be executed.",
          "javascript"),
      std::make_tuple(
          "JavaScript Clipboard Access?",
          "Controls whether JavaScript can access the clipboard.",
          "javascriptAccessClipboard"),
      std::make_tuple(
          "JavaScript DOM Paste?",
          "Controls whether DOM pasting is supported in the editor via "
          "execCommand(\"paste\"). Requires JavaScript clipboaard access.",
          "javascriptDomPaste"),
      std::make_tuple(
          "JavaScript Open Windows?",
          "Controls whether JavaScript can be used for opening windows.",
          "javascriptOpenWindows"),
      std::make_tuple(
          "Local Storage?",
          "Controls whether local storage can be used.",
          "localStorage"),
      std::make_tuple(
          "Browser Plugins?",
          "Controls whether any plugins will be loaded.",
          "plugins"),
      std::make_tuple(
          "Remote Fonts?",
          "Controls the loading of fonts from remote sources.",
          "remoteFonts"),
      std::make_tuple(
          "Tabs to Links?",
          "Controls whether the tab key can advance focus to links.",
          "tabToLinks"),
      std::make_tuple(
          "Text Area Resize?",
          "Controls whether text areas can be resized.",
          "textAreaResize"),
      std::make_tuple(
          "Universal Access From File URLs?",
          "Controls whether file URLs will have access to all URLs.",
          "universalAccessFromFileUrls"),
      std::make_tuple(
          "Web Security?",
          "Controls whether web security restrictions (same-origin policy) "
          "will be enforced.",
          "webSecurity"),
      std::make_tuple(
          "WebGL?",
          "Controls whether WebGL can be used.",
          "webgl")
  });
  QList<QComboBox*> setting_widgs;
  for (int i = 0; i < settings.size(); i++) {
    auto s = settings[i];
    auto widg = new QComboBox;
    widg->addItem("Browser Default", static_cast<int>(STATE_DEFAULT));
    widg->addItem("Enabled", static_cast<int>(STATE_ENABLED));
    widg->addItem("Disabled", static_cast<int>(STATE_DISABLED));
    auto field = std::get<2>(s);
    if (!browser.contains(field)) {
      widg->setCurrentIndex(0);
    } else {
      widg->setCurrentIndex(browser.value(field).toBool() ? 1 : 2);
    }
    create_setting(std::get<0>(s), std::get<1>(s), widg);
    setting_widgs.append(widg);
    if (i < settings.size() - 1) setting_break();
  }

  auto tabs = new QTabWidget;
  auto browser_widg = new QWidget;
  browser_layout->setRowStretch(browser_layout->rowCount(), 1);
  browser_widg->setLayout(browser_layout);
  auto browser_widg_scroll = new QScrollArea;
  browser_widg_scroll->setFrameShape(QFrame::NoFrame);
  browser_widg_scroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
  browser_widg_scroll->setWidget(browser_widg);
  browser_widg_scroll->setWidgetResizable(true);
  tabs->addTab(browser_widg_scroll, "Browser Settings");

  auto dialog_layout = new QGridLayout;
  dialog_layout->addWidget(
        new QLabel(QString("Current Profile: '") + FriendlyName() + "'"),
        0, 0, 1, 2);
  dialog_layout->addWidget(tabs, 1, 0, 1, 2);
  dialog_layout->setColumnStretch(0, 1);
  auto ok = new QPushButton("Save");
  ok->setEnabled(false);
  dialog_layout->addWidget(ok, 2, 0, Qt::AlignRight);
  auto cancel = new QPushButton("Cancel");
  dialog_layout->addWidget(cancel, 2, 1);

  // Change handlers
  auto build_cef = [=]() -> QJsonObject {
    QJsonObject ret;
    if (cache_path_disabled->isChecked()) {
      ret["cachePath"] = "";
    } else if (!cache_path_edit->text().isEmpty()) {
      ret["cachePath"] = cache_path_edit->text();
    }
    if (enable_net_sec->currentData().toBool()) {
      ret["enableNetSecurityExpiration"] = true;
    }
    if (!user_prefs->currentData().toBool()) {
      ret["persistUserPreferences"] = false;
    }
    if (!user_agent_edit->text().isEmpty()) {
      ret["userAgent"] = user_agent_edit->text();
    }
    if (user_data_path_disabled->isChecked()) {
      ret["userDataPath"] = "";
    } else if (!user_data_path_edit->text().isEmpty()) {
      ret["userDataPath"] = user_data_path_edit->text();
    }
    return ret;
  };
  auto build_browser = [=]() -> QJsonObject {
    QJsonObject ret;
    for (int i = 0; i < settings.size(); i++) {
      auto state = static_cast<cef_state_t>(
            setting_widgs[i]->currentData().toInt());
      if (state != STATE_DEFAULT) {
        ret[std::get<2>(settings[i])] = state == STATE_ENABLED;
      }
    }
    return ret;
  };
  auto check_changed = [=]() {
    ok->setEnabled(build_cef() != cef || build_browser() != browser);
    if (ok->isEnabled()) {
      ok->setText("Save and Restart to Apply");
    } else {
      ok->setText("Save");
    }
  };
  connect(cache_path_disabled, &QAbstractButton::toggled, check_changed);
  connect(cache_path_edit, &QLineEdit::textChanged, check_changed);
  connect(enable_net_sec, &QComboBox::currentTextChanged, check_changed);
  connect(user_prefs, &QComboBox::currentTextChanged, check_changed);
  connect(user_agent_edit, &QLineEdit::textChanged, check_changed);
  connect(user_data_path_disabled, &QAbstractButton::toggled, check_changed);
  connect(user_data_path_edit, &QLineEdit::textChanged, check_changed);
  for (const auto& widg : setting_widgs) {
    connect(widg, &QComboBox::currentTextChanged, check_changed);
  }

  QDialog dialog;
  dialog.setWindowTitle("Profile Settings");
  dialog.setLayout(dialog_layout);
  connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
  QSettings geom_settings;
  dialog.restoreGeometry(
        geom_settings.value("profileSettings/geom").toByteArray());
  auto result = dialog.exec();
  geom_settings.setValue("profileSettings/geom", dialog.saveGeometry());
  if (result == QDialog::Rejected) {
    return "";
  }
  wants_restart = ok->text() != "Save";
  cef = build_cef();
  if (!cef.isEmpty()) {
    prefs_["cef"] = cef;
  } else {
    prefs_.remove("cef");
  }
  browser = build_browser();
  if (!browser.isEmpty()) {
    prefs_["browser"] = browser;
  } else {
    prefs_.remove("browser");
  }
  if (!SavePrefs()) {
    QMessageBox::critical(nullptr, "Save Profile", "Error saving profile");
    return "";
  }
  return path_;
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
  QSettings settings("cretz", "Doogie");
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
