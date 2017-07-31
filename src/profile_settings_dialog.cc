#include "profile_settings_dialog.h"
#include "action_manager.h"

namespace doogie {

ProfileSettingsDialog::ProfileSettingsDialog(Profile* profile, QWidget* parent)
    : QDialog(parent), profile_(profile) {

  auto layout = new QGridLayout;
  layout->addWidget(
        new QLabel(QString("Current Profile: '") +
                   profile->FriendlyName() + "'"),
        0, 0, 1, 2);

  auto tabs = new QTabWidget;
  tabs->addTab(CreateSettingsTab(), "Browser Settings");
  tabs->addTab(CreateShortcutTab(), "Keyboard Shortcuts");
  layout->addWidget(tabs, 1, 0, 1, 2);
  layout->setColumnStretch(0, 1);

  auto ok = new QPushButton("Save");
  ok->setEnabled(false);
  layout->addWidget(ok, 2, 0, Qt::AlignRight);
  auto cancel = new QPushButton("Cancel");
  layout->addWidget(cancel, 2, 1);
  setLayout(layout);

  setWindowTitle("Profile Settings");
  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
  connect(this, &ProfileSettingsDialog::SettingsChangeUpdated, [this, ok]() {
    ok->setEnabled(settings_changed_ || shortcuts_changed_);
    if (ok->isEnabled() && NeedsRestart()) {
      ok->setText("Save and Restart to Apply");
    } else {
      ok->setText("Save");
    }
  });

  restoreGeometry(QSettings().value("profileSettings/geom").toByteArray());
}

bool ProfileSettingsDialog::NeedsRestart() {
  return settings_changed_;
}

void ProfileSettingsDialog::done(int r) {
  if (r == Accepted) {
    auto old = profile_->prefs_;
    profile_->prefs_ = BuildPrefsJson();
    if (!profile_->SavePrefs()) {
      QMessageBox::critical(nullptr, "Save Profile", "Error saving profile");
      profile_->prefs_ = old;
      return;
    }
  }
  QDialog::done(r);
}

void ProfileSettingsDialog::closeEvent(QCloseEvent* event) {
  QSettings().setValue("profileSettings/geom", saveGeometry());
  QDialog::closeEvent(event);
}

void ProfileSettingsDialog::keyPressEvent(QKeyEvent* event) {
  // Don't let escape close this
  if (event->key() != Qt::Key_Escape) {
    QDialog::keyPressEvent(event);
  }
}

QWidget* ProfileSettingsDialog::CreateSettingsTab() {
  auto cef = profile_->prefs_.value("cef").toObject();
  auto layout = new QGridLayout;
  layout->setVerticalSpacing(3);
  auto warn_restart =
      new QLabel("NOTE: Changing browser settings requires a restart");
  warn_restart->setStyleSheet("color: red; font-weight: bold");
  layout->addWidget(warn_restart, 0, 0, 1, 2);
  auto create_setting = [layout](
      const QString& name, const QString& desc, QWidget* widg) {
    auto name_label = new QLabel(name);
    name_label->setStyleSheet("font-weight: bold;");
    auto row = layout->rowCount();
    layout->addWidget(name_label, row, 0);
    auto desc_label = new QLabel(desc);
    desc_label->setWordWrap(true);
    layout->addWidget(widg, row, 1);
    layout->addWidget(desc_label, row + 1, 0, 1, 2);
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
  auto setting_break = [layout]() {
    auto frame = new QFrame;
    frame->setFrameShape(QFrame::HLine);
    layout->addWidget(frame, layout->rowCount(), 0, 1, 2);
  };

  auto cache_path_layout = new QHBoxLayout;
  cache_path_disabled_ = new QCheckBox("No cache");
  connect(cache_path_disabled_, &QAbstractButton::toggled,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  cache_path_layout->addWidget(cache_path_disabled_);
  cache_path_edit_ = new QLineEdit;
  cache_path_edit_->setPlaceholderText("Default: PROFILE_DIR/cache");
  connect(cache_path_edit_, &QLineEdit::textChanged,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  cache_path_layout->addWidget(cache_path_edit_, 1);
  auto cache_path_open = new QToolButton();
  cache_path_open->setText("...");
  cache_path_layout->addWidget(cache_path_open);
  auto cache_path_widg = new QWidget;
  cache_path_widg->setLayout(cache_path_layout);
  create_setting("Cache Path",
                 "The location where cache data is stored on disk, if any. ",
                 cache_path_widg);
  connect(cache_path_disabled_, &QCheckBox::toggled,
          [this, cache_path_open](bool checked) {
    cache_path_edit_->setEnabled(!checked);
    cache_path_open->setEnabled(!checked);
  });
  auto curr_cache_path = cef.contains("cachePath") ?
        cef.value("cachePath").toString() : QString();
  cache_path_disabled_->setChecked(!curr_cache_path.isNull() &&
                                   curr_cache_path.isEmpty());
  cache_path_edit_->setText(curr_cache_path);
  setting_break();

  enable_net_sec_ = create_bool_setting(
      "Enable Net Security Expiration",
      "Enable date-based expiration of built in network security information "
      "(i.e. certificate transparency logs, HSTS preloading and pinning "
      "information). Enabling this option improves network security but may "
      "cause HTTPS load failures when using CEF binaries built more than 10 "
      "weeks in the past. See https://www.certificate-transparency.org/ and "
      "https://www.chromium.org/hsts for details.",
      cef.value("enableNetSecurityExpiration").toBool(),
      false);
  connect(enable_net_sec_, &QComboBox::currentTextChanged,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  setting_break();

  user_prefs_ = create_bool_setting(
      "Persist User Preferences",
      "Whether to persist user preferences as a JSON file "
      "in the cache path. Requires cache to be enabled.",
      cef.value("persistUserPreferences").toBool(true),
      true);
  connect(user_prefs_, &QComboBox::currentTextChanged,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  setting_break();

  user_agent_edit_ = new QLineEdit;
  user_agent_edit_->setPlaceholderText("Browser default");
  connect(user_agent_edit_, &QLineEdit::textChanged,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  create_setting("User Agent",
                 "Custom user agent override.",
                 user_agent_edit_);
  if (cef.contains("userAgent")) {
    user_agent_edit_->setText(cef.value("userAgent").toString());
  }
  setting_break();

  auto user_data_path_layout = new QHBoxLayout;
  user_data_path_disabled_ = new QCheckBox("Browser default");
  connect(user_data_path_disabled_, &QAbstractButton::toggled,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  user_data_path_layout->addWidget(user_data_path_disabled_);
  user_data_path_edit_ = new QLineEdit;
  user_data_path_edit_->setPlaceholderText("Default: PROFILE_DIR/user_data");
  connect(user_data_path_edit_, &QLineEdit::textChanged,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  user_data_path_layout->addWidget(user_data_path_edit_, 1);
  auto user_data_path_open = new QToolButton();
  user_data_path_open->setText("...");
  user_data_path_layout->addWidget(user_data_path_open);
  auto user_data_path_widg = new QWidget;
  user_data_path_widg->setLayout(user_data_path_layout);
  create_setting("User Data Path",
                 "The location where user data such as spell checking "
                 "dictionary files will be stored on disk.",
                 user_data_path_widg);
  connect(user_data_path_disabled_, &QCheckBox::toggled,
          [this, user_data_path_open](bool checked) {
    user_data_path_edit_->setEnabled(!checked);
    user_data_path_open->setEnabled(!checked);
  });
  auto curr_user_data_path = cef.contains("userDataPath") ?
        cef.value("userDataPath").toString() : QString();
  user_data_path_disabled_->setChecked(!curr_user_data_path.isNull() &&
                                       curr_user_data_path.isEmpty());
  user_data_path_edit_->setText(curr_user_data_path);

  auto browser = profile_->prefs_.value("browser").toObject();
  for (const auto& setting : Profile::PossibleBrowserSettings()) {
    setting_break();

    auto widg = new QComboBox;
    widg->addItem("Browser Default", static_cast<int>(STATE_DEFAULT));
    widg->addItem("Enabled", static_cast<int>(STATE_ENABLED));
    widg->addItem("Disabled", static_cast<int>(STATE_DISABLED));
    if (!browser.contains(setting.field)) {
      widg->setCurrentIndex(0);
    } else {
      widg->setCurrentIndex(browser.value(setting.field).toBool() ? 1 : 2);
    }
    create_setting(setting.name, setting.desc, widg);
    connect(widg, &QComboBox::currentTextChanged,
            this, &ProfileSettingsDialog::CheckSettingsChange);
    browser_setting_widgs_[setting.name] = widg;
  }

  auto widg = new QWidget;
  layout->setRowStretch(layout->rowCount(), 1);
  widg->setLayout(layout);
  auto widg_scroll = new QScrollArea;
  widg_scroll->setFrameShape(QFrame::NoFrame);
  widg_scroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
  widg_scroll->setWidget(widg);
  widg_scroll->setWidgetResizable(true);
  return widg_scroll;
}

QWidget* ProfileSettingsDialog::CreateShortcutTab() {
  auto layout = new QGridLayout;
  auto table = new QTableWidget;
  table->setColumnCount(3);
  table->setHorizontalHeaderLabels({ "Command", "Label", "Shortcuts" });
  table->verticalHeader()->setVisible(false);
  table->horizontalHeader()->setStretchLastSection(true);
  auto action_meta = QMetaEnum::fromType<ActionManager::Type>();
  for (int i = 0; i < action_meta.keyCount(); i++) {
    auto action = ActionManager::Action(action_meta.value(i));
    if (action) {
      auto row = table->rowCount();
      table->setRowCount(row + 1);
      table->setItem(row, 0, new QTableWidgetItem(action_meta.key(i)));
      table->setItem(row, 1, new QTableWidgetItem(action->text()));
      table->setItem(row, 2, new QTableWidgetItem(
          QKeySequence::listToString(action->shortcuts())));
    }
  }
  layout->addWidget(table, 0, 0);
  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
}

QJsonObject ProfileSettingsDialog::BuildPrefsJson() {
  QJsonObject cef;
  if (cache_path_disabled_->isChecked()) {
    cef["cachePath"] = "";
  } else if (!cache_path_edit_->text().isEmpty()) {
    cef["cachePath"] = cache_path_edit_->text();
  }
  if (enable_net_sec_->currentData().toBool()) {
    cef["enableNetSecurityExpiration"] = true;
  }
  if (!user_prefs_->currentData().toBool()) {
    cef["persistUserPreferences"] = false;
  }
  if (!user_agent_edit_->text().isEmpty()) {
    cef["userAgent"] = user_agent_edit_->text();
  }
  if (user_data_path_disabled_->isChecked()) {
    cef["userDataPath"] = "";
  } else if (!user_data_path_edit_->text().isEmpty()) {
    cef["userDataPath"] = user_data_path_edit_->text();
  }
  QJsonObject browser;
  for (const auto& setting : Profile::PossibleBrowserSettings()) {
    auto state = static_cast<cef_state_t>(
          browser_setting_widgs_[setting.name]->currentData().toInt());
    if (state != STATE_DEFAULT) {
      browser[setting.field] = state == STATE_ENABLED;
    }
  }
  QJsonObject ret;
  if (!cef.isEmpty()) ret["cef"] = cef;
  if (!browser.isEmpty()) ret["browser"] = browser;
  return ret;
}

void ProfileSettingsDialog::CheckSettingsChange() {
  auto orig = settings_changed_;
  auto json = BuildPrefsJson();
  auto orig_cef = profile_->prefs_.value("cef").toObject();
  auto orig_browser = profile_->prefs_.value("browser").toObject();
  settings_changed_ = json.value("cef").toObject() != orig_cef ||
      json.value("browser").toObject() != orig_browser;
  if (orig != settings_changed_) emit SettingsChangeUpdated();
}

}  // namespace doogie
