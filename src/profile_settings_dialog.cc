#include "profile_settings_dialog.h"
#include "action_manager.h"
#include "util.h"
#include "bubble_settings_dialog.h"
#include "settings_widget.h"

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
  tabs->addTab(CreateShortcutsTab(), "Keyboard Shortcuts");
  tabs->addTab(CreateBubblesTab(), "Bubbles");
  layout->addWidget(tabs, 1, 0, 1, 2);
  layout->setColumnStretch(0, 1);

  auto ok = new QPushButton("Save");
  ok->setEnabled(false);
  layout->addWidget(ok, 2, 0, Qt::AlignRight);
  auto cancel = new QPushButton("Cancel");
  layout->addWidget(cancel, 2, 1);
  setLayout(layout);

  setWindowTitle("Profile Settings");
  setSizeGripEnabled(true);
  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
  auto apply_change = [this, ok]() {
    ok->setEnabled(settings_changed_ ||
                   shortcuts_changed_ ||
                   bubbles_changed_);
    if (ok->isEnabled() && NeedsRestart()) {
      ok->setText("Save and Restart to Apply");
    } else {
      ok->setText("Save");
    }
  };
  connect(this, &ProfileSettingsDialog::SettingsChangedUpdated, apply_change);
  connect(this, &ProfileSettingsDialog::ShortcutsChangedUpdated, apply_change);
  connect(this, &ProfileSettingsDialog::BubblesChangedUpdated, apply_change);

  restoreGeometry(QSettings().value("profileSettings/geom").toByteArray());
}

ProfileSettingsDialog::~ProfileSettingsDialog() {
  qDeleteAll(bubbles_);
  bubbles_.clear();
}

bool ProfileSettingsDialog::NeedsRestart() {
  return settings_changed_;
}

void ProfileSettingsDialog::done(int r) {
  if (r == Accepted) {
    auto old = profile_->prefs_;
    profile_->prefs_ = BuildPrefsJson();
    // Also copy all bubbles and delete existing ones
    if (bubbles_changed_) {
      qDeleteAll(profile_->bubbles_);
      profile_->bubbles_.clear();
      for (const auto& bubble : bubbles_) {
        profile_->bubbles_.append(new Bubble(bubble->prefs_, profile_));
      }
    }
    if (!profile_->SavePrefs()) {
      QMessageBox::critical(nullptr, "Save Profile", "Error saving profile");
      profile_->prefs_ = old;
      return;
    }
    // Apply the actions too
    profile_->ApplyActionShortcuts();
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
  auto layout = new QVBoxLayout;
  auto warn_restart =
      new QLabel("NOTE: Changing browser settings requires a restart");
  warn_restart->setStyleSheet("color: red; font-weight: bold");
  layout->addWidget(warn_restart);

  auto settings = new SettingsWidget;
  layout->addWidget(settings);

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
  cache_path_open->setAutoRaise(true);
  cache_path_open->setText("...");
  cache_path_layout->addWidget(cache_path_open);
  auto cache_path_widg = new QWidget;
  cache_path_widg->setLayout(cache_path_layout);
  settings->AddSetting(
        "Cache Path",
        "The location where cache data is stored on disk, if any. ",
        cache_path_widg);
  connect(cache_path_disabled_, &QCheckBox::toggled,
          [this, cache_path_open](bool checked) {
    cache_path_edit_->setEnabled(!checked);
    cache_path_open->setEnabled(!checked);
  });
  connect(cache_path_open, &QToolButton::clicked, [=]() {
    auto existing = cache_path_edit_->text();
    if (existing.isEmpty()) existing = Profile::Current()->Path();
    auto dir = QFileDialog::getExistingDirectory(this,
                                                 "Choose Cache Path",
                                                 existing);
    if (!dir.isEmpty()) cache_path_edit_->setText(dir);
  });
  auto curr_cache_path = cef.contains("cachePath") ?
        cef.value("cachePath").toString() : QString();
  cache_path_disabled_->setChecked(!curr_cache_path.isNull() &&
                                   curr_cache_path.isEmpty());
  cache_path_edit_->setText(curr_cache_path);
  settings->AddSettingBreak();

  enable_net_sec_ = settings->AddYesNoSetting(
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
  settings->AddSettingBreak();

  user_prefs_ = settings->AddYesNoSetting(
      "Persist User Preferences",
      "Whether to persist user preferences as a JSON file "
      "in the cache path. Requires cache to be enabled.",
      cef.value("persistUserPreferences").toBool(true),
      true);
  connect(user_prefs_, &QComboBox::currentTextChanged,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  settings->AddSettingBreak();

  user_agent_edit_ = new QLineEdit;
  user_agent_edit_->setPlaceholderText("Browser default");
  connect(user_agent_edit_, &QLineEdit::textChanged,
          this, &ProfileSettingsDialog::CheckSettingsChange);
  settings->AddSetting(
        "User Agent",
        "Custom user agent override.",
        user_agent_edit_);
  if (cef.contains("userAgent")) {
    user_agent_edit_->setText(cef.value("userAgent").toString());
  }
  settings->AddSettingBreak();

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
  settings->AddSetting(
        "User Data Path",
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
    settings->AddSettingBreak();

    auto index = 0;
    if (browser.contains(setting.field)) {
      index = browser[setting.field].toBool() ? 1 : 2;
    }
    auto box = settings->AddComboBoxSetting(
          setting.name, setting.desc,
          { "Browser Default", "Enabled", "Disabled" },
          index);
    connect(box, &QComboBox::currentTextChanged,
            this, &ProfileSettingsDialog::CheckSettingsChange);
    browser_setting_widgs_[setting.name] = box;
  }

  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
}

QJsonObject ProfileSettingsDialog::BuildCefPrefsJson() {
  QJsonObject ret;
  if (cache_path_disabled_->isChecked()) {
    ret["cachePath"] = "";
  } else if (!cache_path_edit_->text().isEmpty()) {
    ret["cachePath"] = cache_path_edit_->text();
  }
  if (enable_net_sec_->currentIndex() == 0) {
    ret["enableNetSecurityExpiration"] = true;
  }
  if (user_prefs_->currentIndex() == 1) {
    ret["persistUserPreferences"] = false;
  }
  if (!user_agent_edit_->text().isEmpty()) {
    ret["userAgent"] = user_agent_edit_->text();
  }
  if (user_data_path_disabled_->isChecked()) {
    ret["userDataPath"] = "";
  } else if (!user_data_path_edit_->text().isEmpty()) {
    ret["userDataPath"] = user_data_path_edit_->text();
  }
  return ret;
}

QJsonObject ProfileSettingsDialog::BuildBrowserPrefsJson() {
  QJsonObject ret;
  for (const auto& setting : Profile::PossibleBrowserSettings()) {
    auto index = browser_setting_widgs_[setting.name]->currentIndex();
    if (index != 0) {
      ret[setting.field] = index == 1;
    }
  }
  return ret;
}

void ProfileSettingsDialog::CheckSettingsChange() {
  auto orig = settings_changed_;
  auto orig_cef = profile_->prefs_.value("cef").toObject();
  auto orig_browser = profile_->prefs_.value("browser").toObject();
  settings_changed_ = BuildCefPrefsJson() != orig_cef ||
      BuildBrowserPrefsJson() != orig_browser;
  if (orig != settings_changed_) emit SettingsChangedUpdated();
}

QWidget* ProfileSettingsDialog::CreateShortcutsTab() {
  auto layout = new QGridLayout;

  shortcuts_ = new QTableWidget;
  shortcuts_->setColumnCount(3);
  shortcuts_->setSelectionBehavior(QAbstractItemView::SelectRows);
  shortcuts_->setSelectionMode(QAbstractItemView::SingleSelection);
  shortcuts_->setHorizontalHeaderLabels({ "Command", "Label", "Shortcuts" });
  shortcuts_->verticalHeader()->setVisible(false);
  shortcuts_->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
  shortcuts_->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
  shortcuts_->horizontalHeader()->setStretchLastSection(true);
  // <seq, <name>>
  auto known_seqs = new QHash<QString, QStringList>;
  connect(this, &QDialog::destroyed, [known_seqs]() { delete known_seqs; });
  auto action_meta = QMetaEnum::fromType<ActionManager::Type>();
  auto string_item = [](const QString& text) -> QTableWidgetItem* {
    auto item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    return item;
  };
  for (int i = 0; i < action_meta.keyCount(); i++) {
    auto action = ActionManager::Action(action_meta.value(i));
    if (action) {
      auto row = shortcuts_->rowCount();
      shortcuts_->setRowCount(row + 1);
      shortcuts_->setItem(row, 0, string_item(action_meta.key(i)));
      shortcuts_->setItem(row, 1, string_item(action->text()));
      shortcuts_->setItem(row, 2, string_item(
          QKeySequence::listToString(action->shortcuts())));
      for (auto seq : action->shortcuts()) {
        (*known_seqs)[seq.toString()].append(action_meta.key(i));
      }
    }
  }
  shortcuts_->setSortingEnabled(true);
  layout->addWidget(shortcuts_, 0, 0);

  auto edit_group = new QGroupBox;
  edit_group->setVisible(false);
  // We're gonna do this like a "pill box", but no wrapping...
  // we'll just let it scroll horizontal as needed
  auto edit_group_layout = new QGridLayout;
  edit_group_layout->setSizeConstraint(QLayout::SetMinimumSize);
  edit_group->setLayout(edit_group_layout);
  auto list = new QWidget;
  list->setLayout(new QHBoxLayout());
  list->layout()->setSizeConstraint(QLayout::SetMinimumSize);
  auto list_scroll = new QScrollArea();
  list_scroll->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  list_scroll->setFrameShape(QFrame::NoFrame);
  list_scroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
  list_scroll->setWidget(list);
  edit_group_layout->addWidget(list_scroll, 0, 0, 1, 4);
  edit_group_layout->addWidget(new QLabel("New Shortcut"), 1, 0);
  auto shortcut_layout = new QHBoxLayout;
  auto seq_edit = new QKeySequenceEdit;
  seq_edit->setVisible(false);
  shortcut_layout->addWidget(seq_edit, 1);
  auto shortcut_edit = new QLineEdit;
  shortcut_edit->setPlaceholderText("Enter key sequence as text");
  shortcut_layout->addWidget(shortcut_edit, 1);
  auto ok = new QPushButton("Add");
  shortcut_layout->addWidget(ok);
  edit_group_layout->addLayout(shortcut_layout, 1, 1);
  edit_group_layout->setColumnStretch(1, 1);
  auto record = new QPushButton("Record");
  edit_group_layout->addWidget(record, 1, 2);
  auto reset = new QPushButton("Reset");
  edit_group_layout->addWidget(reset, 1, 3);
  auto err = new QLabel;
  err->setStyleSheet("color: red;");
  edit_group_layout->addWidget(err, 2, 0, 1, 4);
  edit_group->adjustSize();

  connect(shortcuts_, &QTableWidget::itemChanged,
          this, &ProfileSettingsDialog::CheckShortcutsChange);

  auto add_seq_item = [this, known_seqs, list](const QKeySequence& seq) {
    auto item = new QWidget;
    item->setObjectName("item");
    item->setStyleSheet(
          "QWidget#item { border: 1px solid black; border-radius: 3px; }");
    item->setLayout(new QHBoxLayout());
    item->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    auto label = new QLabel(seq.toString());
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    item->layout()->addWidget(label);
    auto item_delete = new QToolButton;
    item_delete->setAutoRaise(true);
    item_delete->setText("X");
    item_delete->setStyleSheet("font-weight: bold;");
    item->layout()->addWidget(item_delete);
    connect(item_delete, &QToolButton::clicked,
            [this, known_seqs, seq, item, list]() {
      auto selected = shortcuts_->selectedItems();
      auto seqs = QKeySequence::listFromString(selected[2]->text());
      seqs.removeAll(seq);
      selected[2]->setText(QKeySequence::listToString(seqs));
      item->deleteLater();
      (*known_seqs)[seq.toString()].removeAll(selected[0]->text());
      list->adjustSize();
    });
    list->layout()->addWidget(item);
    list->adjustSize();
  };

  connect(shortcuts_, &QTableWidget::itemSelectionChanged,
          [this, edit_group, list, seq_edit, shortcut_layout,
          shortcut_edit, err, add_seq_item, record]() {
    auto selected = shortcuts_->selectedItems();
    if (selected.size() != 3) {
      edit_group->setVisible(false);
      return;
    }
    record->setText("Record");
    shortcut_layout->itemAt(0)->widget()->hide();
    shortcut_layout->itemAt(1)->widget()->show();
    shortcut_layout->itemAt(2)->widget()->show();
    while (auto item = list->layout()->takeAt(0)) delete item->widget();
    shortcut_edit->clear();
    err->clear();
    edit_group->setVisible(true);
    auto seq_str = selected[2]->text();
    if (!seq_str.isEmpty()) {
      for (const auto& seq : QKeySequence::listFromString(seq_str)) {
        add_seq_item(seq);
      }
    }
  });

  connect(shortcut_edit, &QLineEdit::textChanged,
          [err, ok, known_seqs](const QString& text) {
    err->clear();
    ok->setEnabled(false);
    if (text.isEmpty()) return;
    auto seq = Profile::KeySequenceOrEmpty(text);
    if (seq.isEmpty()) {
      err->setText("Invalid key sequence");
      return;
    }
    ok->setEnabled(true);
    auto existing = known_seqs->value(seq.toString());
    if (!existing.isEmpty()) {
      err->setText(QString("Key sequence exists with other action(s): ")
                   + existing.join(", "));
    }
  });
  auto new_shortcut =
      [this, add_seq_item](const QKeySequence& seq) -> bool {
    if (seq.isEmpty()) return false;
    auto selected = shortcuts_->selectedItems();
    QList<QKeySequence> existing;
    if (!selected[2]->text().isEmpty()) {
      existing = QKeySequence::listFromString(selected[2]->text());
    }
    if (existing.contains(seq)) return false;
    add_seq_item(seq);
    existing.append(seq);
    selected[2]->setText(QKeySequence::listToString(existing));
    return true;
  };
  connect(shortcut_edit, &QLineEdit::returnPressed,
          [shortcut_edit, new_shortcut]() {
    if (new_shortcut(Profile::KeySequenceOrEmpty(shortcut_edit->text()))) {
      shortcut_edit->clear();
    }
  });
  connect(ok, &QPushButton::clicked, [shortcut_edit, new_shortcut]() {
    if (new_shortcut(Profile::KeySequenceOrEmpty(shortcut_edit->text()))) {
      shortcut_edit->clear();
    }
  });
  connect(record, &QPushButton::clicked,
          [record, shortcut_layout, seq_edit]() {
    if (record->text() == "Stop Recording") {
      emit seq_edit->editingFinished();
    } else {
      record->setText("Stop Recording");
      shortcut_layout->itemAt(1)->widget()->hide();
      shortcut_layout->itemAt(2)->widget()->hide();
      shortcut_layout->itemAt(0)->widget()->show();
      seq_edit->clear();
      seq_edit->setFocus();
    }
  });
  connect(seq_edit, &QKeySequenceEdit::editingFinished,
          [record, seq_edit, new_shortcut, shortcut_layout]() {
    record->setText("Record");
    if (!seq_edit->keySequence().isEmpty()) {
      new_shortcut(seq_edit->keySequence());
    }
    seq_edit->clear();
    seq_edit->clearFocus();
    shortcut_layout->itemAt(0)->widget()->hide();
    shortcut_layout->itemAt(1)->widget()->show();
    shortcut_layout->itemAt(2)->widget()->show();
  });
  connect(reset, &QPushButton::clicked,
          [this, record, seq_edit, list, new_shortcut, action_meta]() {
    if (record->text() == "Stop Recording") {
      emit seq_edit->editingFinished();
    }
    // Blow away existing and then update
    auto selected = shortcuts_->selectedItems();
    selected[2]->setText("");
    while (auto item = list->layout()->takeAt(0)) delete item->widget();
    auto action_type = action_meta.keyToValue(
          selected[0]->text().toLocal8Bit().constData());
    for (auto seq : ActionManager::DefaultShortcuts(action_type)) {
      new_shortcut(seq);
    }
  });

  layout->addWidget(edit_group, 1, 0);

  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
}

QJsonObject ProfileSettingsDialog::BuildShortcutsPrefsJson() {
  QJsonObject ret;
  auto action_meta = QMetaEnum::fromType<ActionManager::Type>();
  // Just go over every table item
  for (int i = 0; i < shortcuts_->rowCount(); i++) {
    auto action_type = action_meta.keyToValue(
          shortcuts_->item(i, 0)->text().toLocal8Bit().constData());
    QList<QKeySequence> defs = ActionManager::DefaultShortcuts(action_type);
    QList<QKeySequence> curr;
    auto curr_text = shortcuts_->item(i, 2)->text();
    if (!curr_text.isEmpty()) curr = QKeySequence::listFromString(curr_text);
    if (defs != curr) {
      QJsonArray arr;
      for (auto& seq : curr) {
        arr.append(QJsonValue(seq.toString()));
      }
      ret[action_meta.key(i)] = arr;
    }
  }
  return ret;
}

void ProfileSettingsDialog::CheckShortcutsChange() {
  auto orig = shortcuts_changed_;
  auto orig_shortcuts = profile_->prefs_.value("shortcuts").toObject();
  shortcuts_changed_ = orig_shortcuts != BuildShortcutsPrefsJson();
  if (orig != shortcuts_changed_) emit ShortcutsChangedUpdated();
}

QWidget* ProfileSettingsDialog::CreateBubblesTab() {
  // This is going to be a list of bubbles w/ their icons in tow
  // And the edit screen is a completely separate dialog
  auto layout = new QGridLayout;

  layout->addWidget(
        new QLabel("At least one bubble required. Top bubble is default."),
        0, 0, 1, 5);

  auto list = new QListWidget;
  list->setSelectionBehavior(QAbstractItemView::SelectRows);
  list->setSelectionMode(QAbstractItemView::SingleSelection);
  // Create a copy of each bubble into a local list
  for (const auto& prev_bubble : profile_->Bubbles()) {
    auto bubble = new Bubble(prev_bubble->prefs_);
    bubbles_.append(bubble);
    auto item = new QListWidgetItem(bubble->Icon(), bubble->FriendlyName());
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    list->addItem(item);
  }
  layout->addWidget(list, 1, 0, 1, 5);
  layout->setColumnStretch(0, 1);
  auto new_bubble = new QPushButton("New Bubble");
  layout->addWidget(new_bubble, 2, 0, Qt::AlignRight);
  auto up_bubble = new QPushButton("Move Selected Bubble Up");
  layout->addWidget(up_bubble, 2, 1);
  auto down_bubble = new QPushButton("Move Selected Bubble Down");\
  layout->addWidget(down_bubble, 2, 2);
  auto edit_bubble = new QPushButton("Edit Selected Bubble");
  layout->addWidget(edit_bubble, 2, 3);
  auto delete_bubble = new QPushButton("Delete Selected Bubble");
  layout->addWidget(delete_bubble, 2, 4);

  // row is -1 for new
  auto new_or_edit = [=](int row) {
    QJsonObject json;
    if (row > -1) json = bubbles_[row]->prefs_;
    QStringList invalid_names;
    for (int i = 0; i < bubbles_.size(); i++) {
      if (row != i) invalid_names.append(bubbles_[i]->Name());
    }
    Bubble edit_bubble(json);
    BubbleSettingsDialog bubble_settings(&edit_bubble, invalid_names, this);
    if (bubble_settings.exec() == QDialog::Accepted) {
      if (row == -1) {
        auto bubble = new Bubble(edit_bubble.prefs_);
        bubbles_.append(bubble);
        auto item = new QListWidgetItem(bubble->Icon(), bubble->FriendlyName());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        list->addItem(item);
      } else {
        bubbles_[row]->prefs_ = edit_bubble.prefs_;
        bubbles_[row]->InvalidateIcon();
        auto item = list->item(row);
        item->setText(bubbles_[row]->FriendlyName());
        item->setIcon(bubbles_[row]->Icon());
      }
      CheckBubblesChange();
    }
  };

  auto update_buttons = [=]() {
    if (list->selectedItems().isEmpty()) {
      up_bubble->setEnabled(false);
      down_bubble->setEnabled(false);
      edit_bubble->setEnabled(false);
      delete_bubble->setEnabled(false);
    } else {
      auto item = list->selectedItems().first();
      auto row = list->row(item);
      up_bubble->setEnabled(row > 0);
      down_bubble->setEnabled(row < list->count() - 1);
      edit_bubble->setEnabled(true);
      delete_bubble->setEnabled(list->count() > 1);
    }
  };
  update_buttons();
  connect(list, &QListWidget::itemSelectionChanged, update_buttons);
  connect(list, &QListWidget::itemDoubleClicked, [=](QListWidgetItem* item) {
    new_or_edit(list->row(item));
  });
  connect(new_bubble, &QPushButton::clicked, [=]() {
    new_or_edit(-1);
    update_buttons();
  });
  connect(up_bubble, &QPushButton::clicked, [=]() {
    auto item = list->selectedItems().first();
    auto row = list->row(item);
    bubbles_.move(row, row - 1);
    list->takeItem(row);
    list->insertItem(row - 1, item);
    CheckBubblesChange();
    list->setCurrentItem(item);
  });
  connect(down_bubble, &QPushButton::clicked, [=]() {
    auto item = list->selectedItems().first();
    auto row = list->row(item);
    bubbles_.move(row, row + 1);
    list->takeItem(row);
    list->insertItem(row + 1, item);
    CheckBubblesChange();
    list->setCurrentItem(item);
  });
  connect(edit_bubble, &QPushButton::clicked, [=]() {
    new_or_edit(list->row(list->selectedItems().first()));
    update_buttons();
  });
  connect(delete_bubble, &QPushButton::clicked, [=]() {
    auto row = list->row(list->selectedItems().first());
    delete bubbles_.takeAt(row);
    delete list->takeItem(row);
    CheckBubblesChange();
    update_buttons();
  });

  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
}

void ProfileSettingsDialog::CheckBubblesChange() {
  auto orig = bubbles_changed_;
  bubbles_changed_ = profile_->Bubbles().size() != bubbles_.size();
  if (!bubbles_changed_) {
    for (int i = 0; i < profile_->Bubbles().size(); i++) {
      if (profile_->Bubbles()[i]->prefs_ != bubbles_[i]->prefs_) {
        bubbles_changed_ = true;
        break;
      }
    }
  }
  if (bubbles_changed_ != orig) emit BubblesChangedUpdated();
}

QJsonObject ProfileSettingsDialog::BuildPrefsJson() {
  QJsonObject ret;
  auto cef = BuildCefPrefsJson();
  if (!cef.isEmpty()) ret["cef"] = cef;
  auto browser = BuildBrowserPrefsJson();
  if (!browser.isEmpty()) ret["browser"] = browser;
  auto shortcuts = BuildShortcutsPrefsJson();
  if (!shortcuts.isEmpty()) ret["shortcuts"] = shortcuts;
  return ret;
}

}  // namespace doogie
