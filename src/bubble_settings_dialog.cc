#include "bubble_settings_dialog.h"

#include "settings_widget.h"

namespace doogie {

BubbleSettingsDialog::BubbleSettingsDialog(Bubble* bubble,
                                           QStringList invalid_names,
                                           QWidget* parent)
    : QDialog(parent), bubble_(bubble), invalid_names_(invalid_names) {
  orig_prefs_ = bubble->prefs_;
  ok_ = new QPushButton("OK");
  ok_->setEnabled(false);
  cancel_ = new QPushButton("Cancel");

  auto layout = new QGridLayout;
  auto hr = []() -> QFrame* {
    auto frame = new QFrame;
    frame->setFrameShape(QFrame::HLine);
    return frame;
  };
  layout->addItem(CreateNameSection(), 0, 0, 1, 2);
  layout->addWidget(hr(), 1, 0, 1, 2);
  layout->addItem(CreateIconSection(), 2, 0, 1, 2);
  layout->addWidget(hr(), 3, 0, 1, 2);
  layout->addItem(CreateSettingsSection(), 4, 0, 1, 2);
  layout->addWidget(hr(), 5, 0, 1, 2);
  layout->setColumnStretch(0, 1);
  layout->addWidget(ok_, 6, 0, Qt::AlignRight);
  layout->addWidget(cancel_, 6, 1);

  setWindowTitle("Bubble Settings");
  setSizeGripEnabled(true);
  setLayout(layout);

  connect(ok_, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_, &QPushButton::clicked, this, &QDialog::reject);
  auto check_ok_enabled = [=]() {
    if (!settings_changed_) {
      ok_->setEnabled(false);
    } else {
      auto name_valid = !invalid_names.contains(
            bubble_->prefs_.value("name").toString());
      ok_->setEnabled(name_valid);
    }
  };
  check_ok_enabled();
  connect(this,
          &BubbleSettingsDialog::SettingsChangedUpdated,
          check_ok_enabled);

  restoreGeometry(QSettings().value("bubbleSettings/geom").toByteArray());
}

Bubble* BubbleSettingsDialog::NewBubble(QWidget* parent) {
  QStringList invalid_names;
  for (const auto& bubble : Profile::Current()->Bubbles()) {
    invalid_names.append(bubble->Name());
  }
  QJsonObject json;
  Bubble new_bubble(json);
  BubbleSettingsDialog bubble_settings(&new_bubble, invalid_names, parent);
  if (bubble_settings.exec() == QDialog::Rejected) {
    return nullptr;
  }
  auto bubble = new Bubble(new_bubble.prefs_);
  Profile::Current()->AddBubble(bubble);
  Profile::Current()->SavePrefs();
  return bubble;
}

void BubbleSettingsDialog::closeEvent(QCloseEvent* event) {
  QSettings().setValue("bubbleSettings/geom", saveGeometry());
  QDialog::closeEvent(event);
}

QLayoutItem* BubbleSettingsDialog::CreateNameSection() {
  auto layout = new QHBoxLayout;
  layout->setSpacing(5);
  layout->addWidget(new QLabel("Name:"));
  auto name_edit = new QLineEdit;
  name_edit->setPlaceholderText("(default)");
  name_edit->setText(bubble_->Name());
  layout->addWidget(name_edit, 1);
  auto name_err = new QLabel;
  name_err->setStyleSheet("color: red");
  layout->addWidget(name_err);
  auto name_changed = [=]() {
    if (invalid_names_.contains(name_edit->text())) {
      name_err->setText("Name exists");
    } else {
      name_err->setText("");
    }
    if (name_edit->text().isEmpty()) {
      bubble_->prefs_.remove("name");
    } else {
      bubble_->prefs_["name"] = name_edit->text();
    }
    CheckSettingsChanged();
  };
  name_changed();
  connect(name_edit, &QLineEdit::textChanged, name_changed);
  return layout;
}

QLayoutItem* BubbleSettingsDialog::CreateIconSection() {
  auto layout = new QGridLayout;
  layout->setSpacing(5);
  layout->addWidget(new QLabel("Icon:"), 0, 0, 4, 1, Qt::AlignTop);
  auto hlayout = new QHBoxLayout;
  auto icon_enabled = new QCheckBox;
  hlayout->addWidget(icon_enabled);
  auto icon_pixmap = new QLabel;
  icon_pixmap->setFixedSize(16, 16);
  hlayout->addWidget(icon_pixmap);
  auto icon_label = new QLabel;
  hlayout->addWidget(icon_label, 1);
  layout->addLayout(hlayout, 0, 1);
  layout->setColumnStretch(1, 1);
  hlayout = new QHBoxLayout;
  auto choose_file = new QPushButton("Choose Image File");
  hlayout->addWidget(choose_file);
  auto reset = new QPushButton("Reset to Default");
  hlayout->addWidget(reset);
  hlayout->addStretch(1);
  layout->addLayout(hlayout, 1, 1);
  hlayout = new QHBoxLayout;
  auto icon_color_enabled = new QCheckBox("Color Override");
  hlayout->addWidget(icon_color_enabled);
  auto icon_color = new QToolButton;
  icon_color->setAutoRaise(true);
  hlayout->addWidget(icon_color);
  hlayout->addStretch(1);
  layout->addLayout(hlayout, 2, 1);

  auto update_icon_info = [=]() {
    bubble_->InvalidateIcon();
    icon_pixmap->clear();
    icon_label->clear();
    icon_color_enabled->setChecked(false);
    icon_color->setIcon(QIcon());
    icon_color->setText("");
    if (bubble_->prefs_.contains("icon")) {
      icon_label->setEnabled(true);
      icon_enabled->setChecked(true);
      auto icon = bubble_->Icon();
      if (!icon.isNull()) icon_pixmap->setPixmap(icon.pixmap(16, 16));
      auto text = bubble_->prefs_["icon"].toString();
      if (text.isEmpty()) {
        icon_label->setText("(default)");
      } else {
        icon_label->setText(text);
      }
      choose_file->setEnabled(true);
      reset->setEnabled(true);
      icon_color_enabled->setEnabled(true);
      icon_color->setEnabled(true);
      auto color = bubble_->prefs_.value("color").toString();
      if (QColor::isValidColor(color)) {
        icon_color_enabled->setChecked(true);
        QPixmap pixmap(16, 16);
        pixmap.fill(QColor(color));
        icon_color->setIcon(QIcon(pixmap));
        icon_color->setText(color);
      }
    } else {
      icon_enabled->setChecked(false);
      icon_label->setText("(none)");
      icon_label->setEnabled(false);
      choose_file->setEnabled(false);
      reset->setEnabled(false);
      icon_color_enabled->setEnabled(false);
      icon_color->setEnabled(false);
    }
    CheckSettingsChanged();
  };
  update_icon_info();
  connect(icon_enabled, &QCheckBox::clicked, [=](bool checked) {
    if (checked) {
      bubble_->prefs_["icon"] = "";
    } else {
      bubble_->prefs_.remove("icon");
    }
    update_icon_info();
  });
  connect(choose_file, &QPushButton::clicked, [=]() {
    auto filter = "Images (*.bmp *.gif *.jpg *.jpeg *.png "
                  "*.pbm *.pgm *.ppm *.xbm *.xpm *.svg)";
    auto existing_dir =
        QSettings().value("bubbleSettings/iconFileOpen").toString();
    auto icon_path = bubble_->prefs_.value("icon").toString();
    if (!icon_path.isEmpty()) {
      existing_dir = QFileInfo(icon_path).dir().path();
    }
    if (existing_dir.isEmpty()) {
      existing_dir = QStandardPaths::writableLocation(
          QStandardPaths::PicturesLocation);
    }
    auto file = QFileDialog::getOpenFileName(this, "Choose Image File",
                                             existing_dir, filter);
    if (!file.isEmpty() && !QImageReader::imageFormat(file).isEmpty()) {
      bubble_->prefs_["icon"] = file;
      update_icon_info();
      QSettings().setValue("bubbleSettings/iconFileOpen",
                           QFileInfo(file).dir().path());
    }
  });
  connect(reset, &QPushButton::clicked, [=]() {
    bubble_->prefs_["icon"] = "";
    update_icon_info();
  });
  connect(icon_color_enabled, &QCheckBox::clicked, [=](bool checked) {
    if (checked) {
      bubble_->prefs_["color"] = "black";
    } else {
      bubble_->prefs_.remove("color");
    }
    update_icon_info();
  });
  connect(icon_color, &QToolButton::clicked, [=]() {
    auto color = QColorDialog::getColor(
          QColor(bubble_->prefs_["color"].toString()),
          this, "Bubble Color Override");
    if (color.isValid()) {
      bubble_->prefs_["color"] = color.name();
      update_icon_info();
    }
  });

  return layout;
}

QLayoutItem* BubbleSettingsDialog::CreateSettingsSection() {
  auto layout = new QVBoxLayout;
  auto warn_restart =
      new QLabel("NOTE: Most changes to these settings only apply to new "
                 "pages in this bubble, not current open ones.");
  warn_restart->setStyleSheet("color: red; font-weight: bold");
  layout->addWidget(warn_restart);

  auto settings = new SettingsWidget;
  layout->addWidget(settings);

  auto cef = bubble_->prefs_.value("cef").toObject();

  auto cache_path_layout = new QHBoxLayout;
  auto cache_path_default = new QCheckBox("Same as profile");
  cache_path_layout->addWidget(cache_path_default);
  auto cache_path_disabled = new QCheckBox("No cache");
  cache_path_layout->addWidget(cache_path_disabled);
  auto cache_path_edit = new QLineEdit;
  cache_path_edit->setPlaceholderText("Default: PROFILE_DIR/cache");
  cache_path_layout->addWidget(cache_path_edit, 1);
  auto cache_path_open = new QToolButton();
  cache_path_open->setAutoRaise(true);
  cache_path_open->setText("...");
  cache_path_layout->addWidget(cache_path_open);
  auto cache_path_widg = new QWidget;
  cache_path_widg->setLayout(cache_path_layout);
  connect(cache_path_default, &QCheckBox::toggled, [=](bool checked) {
    cache_path_disabled->setEnabled(!checked);
    cache_path_edit->setEnabled(!checked && !cache_path_disabled->isChecked());
    cache_path_open->setEnabled(!checked && !cache_path_disabled->isChecked());
  });
  connect(cache_path_disabled, &QCheckBox::toggled, [=](bool checked) {
    cache_path_edit->setEnabled(!checked && !cache_path_default->isChecked());
    cache_path_open->setEnabled(!checked && !cache_path_default->isChecked());
  });
  connect(cache_path_open, &QToolButton::clicked, [=]() {
    auto existing = cache_path_edit->text();
    if (existing.isEmpty()) {
      existing = QSettings().value("bubbleSettings/cachePathOpen").toString();
    }
    if (existing.isEmpty()) existing = Profile::Current()->Path();
    auto dir = QFileDialog::getExistingDirectory(this,
                                                 "Choose Cache Path",
                                                 existing);
    if (!dir.isEmpty()) {
      QSettings().setValue("bubbleSettings/cachePathOpen", dir);
      cache_path_edit->setText(dir);
    }
  });
  if (!cef.contains("cachePath")) {
    cache_path_default->setChecked(true);
  } else {
    cache_path_default->setChecked(false);
    if (cef["cachePath"].isNull()) {
      cache_path_disabled->setChecked(true);
    } else {
      cache_path_edit->setText(cef["cachePath"].toString());
    }
  }

  settings->AddSetting(
        "Cache Path",
        "The location where cache data is stored on disk, if any. ",
        cache_path_widg);
  settings->AddSettingBreak();

  auto enable_net_sec_index = 0;
  if (cef.contains("enableNetSecurityExpiration")) {
    enable_net_sec_index = cef["enableNetSecurityExpiration"].toBool() ? 1 : 0;
  }
  auto enable_net_sec = settings->AddComboBoxSetting(
      "Enable Net Security Expiration",
      "Enable date-based expiration of built in network security information "
      "(i.e. certificate transparency logs, HSTS preloading and pinning "
      "information). Enabling this option improves network security but may "
      "cause HTTPS load failures when using CEF binaries built more than 10 "
      "weeks in the past. See https://www.certificate-transparency.org/ and "
      "https://www.chromium.org/hsts for details.",
      { "Same as profile", "Enabled", "Disabled" },
      enable_net_sec_index);

  auto user_pref_index = 0;
  if (cef.contains("persistUserPreferences")) {
    user_pref_index = cef["persistUserPreferences"].toBool() ? 1 : 0;
  }
  auto user_prefs = settings->AddComboBoxSetting(
        "Persist User Preferences",
        "Whether to persist user preferences as a JSON file "
        "in the cache path. Requires cache to be enabled.",
        { "Same as profile", "Enabled", "Disabled" },
        user_pref_index);
  settings->AddSettingBreak();

  auto browser = bubble_->prefs_.value("browser").toObject();
  QHash<QString, QComboBox*> browser_settings;
  for (const auto& setting : Profile::PossibleBrowserSettings()) {
    settings->AddSettingBreak();
    auto index = 0;
    if (browser.contains(setting.field)) {
      index = browser[setting.field].toBool() ? 1 : 0;
    }
    browser_settings[setting.name] = settings->AddComboBoxSetting(
          setting.name, setting.desc,
          { "Same as profile", "Enabled", "Disabled" },
          index);
  }

  auto update_cef_prefs = [=]() {
    QJsonObject cef;
    if (!cache_path_default->isChecked()) {
      if (cache_path_disabled->isChecked()) {
        cef["cachePath"] = QJsonValue();
      } else {
        cef["cachePath"] = cache_path_edit->text();
      }
    }
    if (enable_net_sec->currentIndex() > 0) {
      cef["enableNetSecurityExpiration"] = enable_net_sec->currentIndex() == 1;
    }
    if (user_prefs->currentIndex() > 0) {
      cef["persistUserPreferences"] = user_prefs->currentIndex() == 1;
    }
    if (cef.isEmpty()) {
      bubble_->prefs_.remove("cef");
    } else {
      bubble_->prefs_["cef"] = cef;
    }
    CheckSettingsChanged();
  };

  connect(cache_path_default, &QCheckBox::clicked, update_cef_prefs);
  connect(cache_path_disabled, &QCheckBox::clicked, update_cef_prefs);
  connect(cache_path_edit, &QLineEdit::textChanged, update_cef_prefs);
  connect(enable_net_sec, &QComboBox::currentTextChanged, update_cef_prefs);
  connect(user_prefs, &QComboBox::currentTextChanged, update_cef_prefs);

  auto update_browser_prefs = [=]() {
    QJsonObject browser;
    for (const auto& setting : Profile::PossibleBrowserSettings()) {
      auto index = browser_settings[setting.name]->currentIndex();
      if (index > 0) browser[setting.field] = index == 1;
    }
    if (browser.isEmpty()) {
      bubble_->prefs_.remove("browser");
    } else {
      bubble_->prefs_["browser"] = browser;
    }
    CheckSettingsChanged();
  };
  for (const auto& setting : Profile::PossibleBrowserSettings()) {
    connect(browser_settings[setting.name],
            &QComboBox::currentTextChanged,
            update_browser_prefs);
  }

  return layout;
}

void BubbleSettingsDialog::CheckSettingsChanged() {
  auto orig = settings_changed_;
  settings_changed_ = orig_prefs_ != bubble_->prefs_;
  if (settings_changed_ != orig) emit SettingsChangedUpdated();
}

}  // namespace doogie
