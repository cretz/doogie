#include "profile_settings_dialog.h"

#include "action_manager.h"
#include "bubble_settings_dialog.h"
#include "settings_widget.h"
#include "util.h"

namespace doogie {

ProfileSettingsDialog::ProfileSettingsDialog(Profile* profile,
                                             QSet<QString> in_use_bubble_names,
                                             QWidget* parent)
    : QDialog(parent),
      orig_profile_(profile),
      in_use_bubble_names_(in_use_bubble_names) {
  profile_.CopySettingsFrom(*profile);
  temp_bubbles_ = profile_.Bubbles();
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
  connect(this, &ProfileSettingsDialog::Changed, [=]() {
    ok->setEnabled(*orig_profile_ != profile_);
    if (ok->isEnabled() &&
        orig_profile_->RequiresRestartIfChangedTo(profile_)) {
      ok->setText("Save and Restart to Apply");
    } else {
      ok->setText("Save");
    }
  });

  restoreGeometry(QSettings().value("profileSettings/geom").toByteArray());
}

void ProfileSettingsDialog::done(int r) {
  if (r == Accepted) {
    Profile orig = *orig_profile_;
    needs_restart_ = orig_profile_->RequiresRestartIfChangedTo(profile_);
    orig_profile_->CopySettingsFrom(profile_);
    if (!orig_profile_->SavePrefs()) {
      QMessageBox::critical(nullptr, "Save Profile", "Error saving profile");
      orig_profile_->CopySettingsFrom(orig);
      return;
    }
    // Apply the actions too
    orig_profile_->ApplyActionShortcuts();
  }
  QDialog::done(r);
}

void ProfileSettingsDialog::closeEvent(QCloseEvent* event) {
  QSettings().setValue("profileSettings/geom", saveGeometry());
  QDialog::closeEvent(event);
}

void ProfileSettingsDialog::keyPressEvent(QKeyEvent* event) {
  // Don't let escape close this if there are changes
  if (event->key() == Qt::Key_Escape && *orig_profile_ != profile_) {
    event->ignore();
    return;
  }
  QDialog::keyPressEvent(event);
}

QWidget* ProfileSettingsDialog::CreateSettingsTab() {
  auto layout = new QVBoxLayout;
  auto warn_restart = new QLabel(
        "NOTE: Changing some browser settings will request a restart");
  warn_restart->setStyleSheet("color: red; font-weight: bold");
  layout->addWidget(warn_restart);

  auto settings = new SettingsWidget;
  layout->addWidget(settings);

  auto cache_path_layout = new QHBoxLayout;
  auto cache_path_disabled = new QCheckBox("No cache");
  auto cache_path = profile_.CachePath();
  cache_path_disabled->setChecked(!cache_path.isNull() && cache_path.isEmpty());
  cache_path_layout->addWidget(cache_path_disabled);
  auto cache_path_edit = new QLineEdit;
  cache_path_edit->setEnabled(!cache_path_disabled->isEnabled());
  cache_path_edit->setPlaceholderText("Default: PROFILE_DIR/cache");
  cache_path_edit->setText(cache_path);
  cache_path_layout->addWidget(cache_path_edit, 1);
  auto cache_path_open = new QToolButton();
  cache_path_open->setAutoRaise(true);
  cache_path_open->setText("...");
  cache_path_open->setEnabled(!cache_path_disabled->isEnabled());
  cache_path_layout->addWidget(cache_path_open);
  auto cache_path_widg = new QWidget;
  cache_path_widg->setLayout(cache_path_layout);
  settings->AddSetting(
        "Cache Path",
        "The location where cache data is stored on disk, if any. ",
        cache_path_widg);
  auto cache_path_update = [=]() {
    cache_path_edit->setEnabled(!cache_path_disabled->isChecked());
    cache_path_open->setEnabled(!cache_path_disabled->isChecked());

    if (cache_path_disabled->isChecked()) {
      profile_.SetCachePath("");
    } else {
      auto text = cache_path_edit->text();
      // Empty means null for us
      profile_.SetCachePath(text.isEmpty() ? QString() : text);
    }
    emit Changed();
  };
  connect(cache_path_disabled, &QCheckBox::toggled, cache_path_update);
  connect(cache_path_edit, &QLineEdit::textChanged, cache_path_update);
  connect(cache_path_open, &QToolButton::clicked, [=]() {
    auto existing = cache_path_edit->text();
    if (existing.isEmpty()) {
      existing = QSettings().value("profileSettings/cachePathOpen").toString();
    }
    if (existing.isEmpty()) existing = Profile::Current()->Path();
    auto dir = QFileDialog::getExistingDirectory(this,
                                                 "Choose Cache Path",
                                                 existing);
    if (!dir.isEmpty()) {
      QSettings().setValue("profileSettings/cachePathOpen", dir);
      cache_path_edit->setText(dir);
    }
  });
  settings->AddSettingBreak();

  auto enable_net_sec = settings->AddYesNoSetting(
      "Enable Net Security Expiration",
      "Enable date-based expiration of built in network security information "
      "(i.e. certificate transparency logs, HSTS preloading and pinning "
      "information). Enabling this option improves network security but may "
      "cause HTTPS load failures when using CEF binaries built more than 10 "
      "weeks in the past. See https://www.certificate-transparency.org/ and "
      "https://www.chromium.org/hsts for details.",
      profile_.EnableNetSec(),
      false);
  connect(enable_net_sec, &QComboBox::currentTextChanged, [=]() {
    profile_.SetEnableNetSec(enable_net_sec->currentIndex() == 0);
    emit Changed();
  });
  settings->AddSettingBreak();

  auto user_prefs = settings->AddYesNoSetting(
      "Persist User Preferences",
      "Whether to persist user preferences as a JSON file "
      "in the cache path. Requires cache to be enabled.",
      profile_.PersistUserPrefs(),
      true);
  connect(user_prefs, &QComboBox::currentTextChanged, [=]() {
    profile_.SetPersistUserPrefs(user_prefs->currentIndex() == 0);
    emit Changed();
  });
  settings->AddSettingBreak();

  auto user_agent_edit = new QLineEdit;
  user_agent_edit->setPlaceholderText("Browser default");
  user_agent_edit->setText(profile_.UserAgent());
  connect(user_agent_edit, &QLineEdit::textChanged, [=]() {
    profile_.SetUserAgent(user_agent_edit->text());
    emit Changed();
  });
  settings->AddSetting(
        "User Agent",
        "Custom user agent override.",
        user_agent_edit);
  settings->AddSettingBreak();

  auto user_data_path_layout = new QHBoxLayout;
  auto user_data_path = profile_.UserDataPath();
  auto user_data_path_disabled = new QCheckBox("Browser default");
  user_data_path_disabled->setChecked(
        !user_data_path.isNull() && user_data_path.isEmpty());
  user_data_path_layout->addWidget(user_data_path_disabled);
  auto user_data_path_edit = new QLineEdit;
  user_data_path_edit->setPlaceholderText("Default: PROFILE_DIR/user_data");
  user_data_path_edit->setEnabled(!user_data_path_disabled->isChecked());
  user_data_path_edit->setText(user_data_path);
  user_data_path_layout->addWidget(user_data_path_edit, 1);
  auto user_data_path_open = new QToolButton();
  user_data_path_open->setText("...");
  user_data_path_open->setEnabled(!user_data_path_disabled->isChecked());
  user_data_path_layout->addWidget(user_data_path_open);
  auto user_data_path_widg = new QWidget;
  user_data_path_widg->setLayout(user_data_path_layout);
  settings->AddSetting(
        "User Data Path",
        "The location where user data such as spell checking "
        "dictionary files will be stored on disk.",
        user_data_path_widg);
  auto user_data_path_update = [=]() {
    user_data_path_edit->setEnabled(!user_data_path_disabled->isChecked());
    user_data_path_open->setEnabled(!user_data_path_disabled->isChecked());

    if (user_data_path_disabled->isChecked()) {
      profile_.SetUserDataPath("");
    } else {
      auto text = user_data_path_edit->text();
      // Empty means null for us
      profile_.SetUserDataPath(text.isEmpty() ? QString() : text);
    }
    emit Changed();
  };
  connect(user_data_path_disabled,
          &QCheckBox::toggled,
          user_data_path_update);
  connect(user_data_path_edit,
          &QLineEdit::textChanged,
          user_data_path_update);
  connect(user_data_path_open, &QToolButton::clicked, [=]() {
    auto existing = user_data_path_edit->text();
    if (existing.isEmpty()) {
      existing = QSettings().value("profileSettings/userDataPathOpen").toString();
    }
    if (existing.isEmpty()) existing = Profile::Current()->Path();
    auto dir = QFileDialog::getExistingDirectory(this,
                                                 "Choose User Data Path",
                                                 existing);
    if (!dir.isEmpty()) {
      QSettings().setValue("profileSettings/userDataPathOpen", dir);
      user_data_path_edit->setText(dir);
    }
  });

  for (auto& setting : Profile::PossibleBrowserSettings()) {
    settings->AddSettingBreak();

    auto box = settings->AddComboBoxSetting(
          setting.name, setting.desc,
          { "Browser Default", "Enabled", "Disabled" },
          profile_.GetBrowserSetting(setting.field));
    connect(box, &QComboBox::currentTextChanged, [=]() {
      profile_.SetBrowserSetting(
            setting.field,
            static_cast<Util::SettingState>(box->currentIndex()));
      emit Changed();
    });
  }

  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
}

QWidget* ProfileSettingsDialog::CreateShortcutsTab() {
  auto layout = new QGridLayout;

  auto shortcuts = new QTableWidget;
  shortcuts->setColumnCount(3);
  shortcuts->setSelectionBehavior(QAbstractItemView::SelectRows);
  shortcuts->setSelectionMode(QAbstractItemView::SingleSelection);
  shortcuts->setHorizontalHeaderLabels({ "Command", "Label", "Shortcuts" });
  shortcuts->verticalHeader()->setVisible(false);
  shortcuts->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
  shortcuts->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
  shortcuts->horizontalHeader()->setStretchLastSection(true);
  // <seq, <name>>
  auto known_seqs = new QHash<QString, QStringList>;
  connect(this, &QDialog::destroyed, [=]() { delete known_seqs; });
  auto string_item = [](const QString& text) -> QTableWidgetItem* {
    auto item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    return item;
  };
  auto action_types = ActionManager::Actions().keys();
  for (int i = 0; i < action_types.size(); i++) {
    auto type = action_types[i];
    auto action = ActionManager::Action(type);
    if (action) {
      auto row = shortcuts->rowCount();
      shortcuts->setRowCount(row + 1);
      shortcuts->setItem(row, 0,
                         string_item(ActionManager::TypeToString(type)));
      shortcuts->setItem(row, 1, string_item(action->text()));
      shortcuts->setItem(row, 2, string_item(
          QKeySequence::listToString(action->shortcuts())));
      for (auto seq : action->shortcuts()) {
        (*known_seqs)[seq.toString()].append(
              ActionManager::TypeToString(type));
      }
    }
  }
  shortcuts->setSortingEnabled(true);
  layout->addWidget(shortcuts, 0, 0);

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

  connect(shortcuts, &QTableWidget::itemChanged, [=]() {
    // Just build all the shortcuts
    QHash<int, QList<QKeySequence>> new_shortcuts;
    for (int i = 0; i < shortcuts->rowCount(); i++) {
      auto action_type = ActionManager::StringToType(
            shortcuts->item(i, 0)->text());
      auto defaults = ActionManager::DefaultShortcuts(action_type);
      QList<QKeySequence> curr;
      auto curr_text = shortcuts->item(i, 2)->text();
      if (!curr_text.isEmpty()) curr = QKeySequence::listFromString(curr_text);
      if (defaults != curr) {
        new_shortcuts[action_type] = curr;
      }
    }
    profile_.SetShortcuts(new_shortcuts);
    emit Changed();
  });

  auto add_seq_item = [=](const QKeySequence& seq) {
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
    connect(item_delete, &QToolButton::clicked, [=]() {
      auto selected = shortcuts->selectedItems();
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

  connect(shortcuts, &QTableWidget::itemSelectionChanged, [=]() {
    auto selected = shortcuts->selectedItems();
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

  connect(shortcut_edit, &QLineEdit::textChanged, [=](const QString& text) {
    err->clear();
    ok->setEnabled(false);
    if (text.isEmpty()) return;
    auto seq = Util::KeySequenceOrEmpty(text);
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
  auto new_shortcut = [=](const QKeySequence& seq) -> bool {
    if (seq.isEmpty()) return false;
    auto selected = shortcuts->selectedItems();
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
  connect(shortcut_edit, &QLineEdit::returnPressed, [=]() {
    if (new_shortcut(Util::KeySequenceOrEmpty(shortcut_edit->text()))) {
      shortcut_edit->clear();
    }
  });
  connect(ok, &QPushButton::clicked, [=]() {
    if (new_shortcut(Util::KeySequenceOrEmpty(shortcut_edit->text()))) {
      shortcut_edit->clear();
    }
  });
  connect(record, &QPushButton::clicked, [=]() {
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
  connect(seq_edit, &QKeySequenceEdit::editingFinished, [=]() {
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
  connect(reset, &QPushButton::clicked, [=]() {
    if (record->text() == "Stop Recording") {
      emit seq_edit->editingFinished();
    }
    // Blow away existing and then update
    auto selected = shortcuts->selectedItems();
    selected[2]->setText("");
    while (auto item = list->layout()->takeAt(0)) delete item->widget();
    auto action_type = ActionManager::StringToType(selected[0]->text());
    for (auto seq : ActionManager::DefaultShortcuts(action_type)) {
      new_shortcut(seq);
    }
  });

  layout->addWidget(edit_group, 1, 0);

  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
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
  for (auto& bubble : profile_.Bubbles()) {
    auto item = new QListWidgetItem(bubble.Icon(), bubble.FriendlyName());
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (in_use_bubble_names_.contains(bubble.Name())) {
      item->setText(item->text() + "*");
    }
    list->addItem(item);
  }
  layout->addWidget(list, 1, 0, 1, 5);
  layout->setColumnStretch(0, 1);
  auto in_use_label = new QLabel(
      "* - Bubbles on existing pages cannot be edited or deleted.");
  layout->addWidget(in_use_label, 2, 0, 1, 5);
  if (in_use_bubble_names_.isEmpty()) in_use_label->setVisible(false);
  auto new_bubble = new QPushButton("New Bubble");
  layout->addWidget(new_bubble, 3, 0, Qt::AlignRight);
  auto up_bubble = new QPushButton("Move Selected Bubble Up");
  layout->addWidget(up_bubble, 3, 1);
  auto down_bubble = new QPushButton("Move Selected Bubble Down");\
  layout->addWidget(down_bubble, 3, 2);
  auto edit_bubble = new QPushButton("Edit Selected Bubble");
  layout->addWidget(edit_bubble, 3, 3);
  auto delete_bubble = new QPushButton("Delete Selected Bubble");
  layout->addWidget(delete_bubble, 3, 4);

  auto bubbles_changed = [=]() {
    profile_.SetBubbles(temp_bubbles_);
    emit Changed();
  };

  // row is -1 for new
  auto new_or_edit = [=](int row) {
    QSet<QString> invalid_names;
    for (int i = 0; i < temp_bubbles_.size(); i++) {
      if (row != i) invalid_names.insert(temp_bubbles_[i].Name());
    }
    Bubble bubble;
    if (row > -1) bubble = temp_bubbles_[row];
    if (BubbleSettingsDialog::UpdateBubble(&bubble, invalid_names, this)) {
      if (row == -1) {
        auto item = new QListWidgetItem(bubble.Icon(), bubble.FriendlyName());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        list->addItem(item);
        temp_bubbles_.append(bubble);
      } else {
        auto item = list->item(row);
        item->setText(bubble.FriendlyName());
        item->setIcon(bubble.Icon());
        temp_bubbles_[row] = bubble;
      }
      bubbles_changed();
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
      auto editable = !in_use_bubble_names_.contains(temp_bubbles_[row].Name());
      edit_bubble->setEnabled(editable);
      delete_bubble->setEnabled(editable && list->count() > 1);
    }
  };
  update_buttons();
  connect(list, &QListWidget::itemSelectionChanged, update_buttons);
  connect(list, &QListWidget::itemDoubleClicked, [=](QListWidgetItem* item) {
    auto row = list->row(item);
    if (!in_use_bubble_names_.contains(temp_bubbles_[row].Name())) {
      new_or_edit(row);
    }
  });
  connect(new_bubble, &QPushButton::clicked, [=]() {
    new_or_edit(-1);
    update_buttons();
  });
  connect(up_bubble, &QPushButton::clicked, [=]() {
    auto item = list->selectedItems().first();
    auto row = list->row(item);
    list->takeItem(row);
    list->insertItem(row - 1, item);
    list->setCurrentItem(item);
    temp_bubbles_.move(row, row - 1);
    bubbles_changed();
  });
  connect(down_bubble, &QPushButton::clicked, [=]() {
    auto item = list->selectedItems().first();
    auto row = list->row(item);
    list->takeItem(row);
    list->insertItem(row + 1, item);
    list->setCurrentItem(item);
    temp_bubbles_.move(row, row + 1);
    bubbles_changed();
  });
  connect(edit_bubble, &QPushButton::clicked, [=]() {
    new_or_edit(list->row(list->selectedItems().first()));
    update_buttons();
  });
  connect(delete_bubble, &QPushButton::clicked, [=]() {
    auto row = list->row(list->selectedItems().first());
    delete list->takeItem(row);
    temp_bubbles_.removeAt(row);
    bubbles_changed();
    update_buttons();
  });

  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
}

}  // namespace doogie
