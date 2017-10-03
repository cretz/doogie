#include "profile_settings_dialog.h"

#include "action_manager.h"
#include "browser_setting.h"
#include "bubble_settings_dialog.h"
#include "settings_widget.h"
#include "util.h"

namespace doogie {

ProfileSettingsDialog::ProfileSettingsDialog(const Cef& cef,
                                             QSet<qlonglong> in_use_bubble_ids,
                                             QWidget* parent)
    : QDialog(parent),
      in_use_bubble_ids_(in_use_bubble_ids),
      profile_(Profile::Current()) {
  setWindowModality(Qt::WindowModal);
  temp_bubbles_ = Bubble::CachedBubbles();
  auto layout = new QGridLayout;
  layout->addWidget(
        new QLabel(QString("Current Profile: '") +
                   Profile::Current().FriendlyName() + "'"),
        0, 0, 1, 2);

  tabs_ = new QTabWidget;
  tabs_->addTab(CreateSettingsTab(), "Browser Settings");
  tabs_->addTab(CreateShortcutsTab(), "Keyboard Shortcuts");
  tabs_->addTab(CreateBubblesTab(), "Bubbles");
  tabs_->addTab(CreateBlockerTab(cef), "Blocker Lists");
  layout->addWidget(tabs_, 1, 0, 1, 2);
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
    ok->setEnabled(SettingsChanged());
    if (ok->isEnabled() &&
        Profile::Current().RequiresRestartIfChangedTo(profile_)) {
      ok->setText("Save and Restart to Apply");
    } else {
      ok->setText("Save");
    }
  });

  restoreGeometry(QSettings().value("profileSettings/geom").toByteArray());
}

void ProfileSettingsDialog::done(int r) {
  if (r == Accepted) {\
    needs_restart_ = Profile::Current().RequiresRestartIfChangedTo(profile_);

    // Save in a transaction
    auto db = QSqlDatabase::database();
    if (!db.transaction()) {
      qCritical() << "Transaction creation failed";
      return;
    }
    if (!Save()) {
      qCritical() << "Unable to save profile";
      db.rollback();
      return;
    }
    if (!db.commit()) {
      qCritical() << "Unable to commit profile";
      return;
    }

    // Apply the actions
    Profile::Current().ApplyActionShortcuts();
  }
  QDialog::done(r);
}

void ProfileSettingsDialog::closeEvent(QCloseEvent* event) {
  QSettings().setValue("profileSettings/geom", saveGeometry());
  QDialog::closeEvent(event);
}

void ProfileSettingsDialog::keyPressEvent(QKeyEvent* event) {
  // Don't let escape close this if there are changes
  if (event->key() == Qt::Key_Escape && SettingsChanged()) {
    event->ignore();
    return;
  }
  QDialog::keyPressEvent(event);
}

bool ProfileSettingsDialog::Save() {
  // Save the rule list stuff
  QSet<qlonglong> enabled_blocker_list_ids;
  QSet<qlonglong> all_list_ids;
  for (auto& list : temp_blocker_lists_.values()) {
    if (list.Exists() &&
        blocker_list_ids_pending_refresh_.contains(list.Id())) {
      list.SetLastRefreshed(QDateTime());
    }
    if (!list.Persist()) return false;
    all_list_ids << list.Id();
    if (enabled_blocker_lists_.contains(list.UrlOrLocalPath())) {
      enabled_blocker_list_ids << list.Id();
    }
  }
  profile_.SetEnabledBlockerListIds(enabled_blocker_list_ids);

  // Save the bubble stuff...
  if (!Bubble::ResetOrderIndexes()) return false;
  QSet<qlonglong> existing_ids;
  for (int i = 0; i < temp_bubbles_.size(); i++) {
    temp_bubbles_[i].SetOrderIndex(i);
    if (!temp_bubbles_[i].Persist()) return false;
    existing_ids << temp_bubbles_[i].Id();
  }
  // Delete the ones no longer there
  for (auto& bubble : Bubble::CachedBubbles()) {
    if (!existing_ids.contains(bubble.Id())) {
      if (!Workspace::WorkspacePage::BubbleDeleted(bubble.Id())) return false;
      if (!bubble.Delete()) return false;
    }
  }
  Bubble::InvalidateCachedBubbles();

  // Save the profile stuff (ignore failures for now...)
  Profile::Current().CopySettingsFrom(profile_);
  if (!Profile::Current().Persist()) return false;

  // Delete the lists no longer there
  for (auto& list : orig_blocker_lists_) {
    if (list.Exists() && !all_list_ids.contains(list.Id())) {
      if (!list.Delete()) return false;
    }
  }
  return true;
}

bool ProfileSettingsDialog::SettingsChanged() const {
  return Profile::Current() != profile_ ||
      temp_bubbles_ != Bubble::CachedBubbles() ||
      BlockerChanged();
}

bool ProfileSettingsDialog::BlockerChanged() const {
  if (!blocker_list_ids_pending_refresh_.isEmpty() ||
      orig_blocker_lists_.size() != temp_blocker_lists_.size()) {
    return true;
  }
  // They can change what is checked
  if (enabled_blocker_lists_.size() !=
      profile_.EnabledBlockerListIds().size()) {
    return true;
  }
  for (const auto& enabled_key : enabled_blocker_lists_) {
    auto list = temp_blocker_lists_[enabled_key];
    if (!list.Exists()) return true;
    if (!profile_.EnabledBlockerListIds().contains(list.Id())) {
      return true;
    }
  }

  for (const auto& orig_list : orig_blocker_lists_) {
    auto temp_list = temp_blocker_lists_.value(orig_list.UrlOrLocalPath());
    // IDs have to match
    if (orig_list.Id() != temp_list.Id()) return true;
    // They can only change the name on local ones
    if (orig_list.IsLocalOnly() && orig_list.Name() != temp_list.Name()) {
      return true;
    }
  }
  return false;
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
    if (existing.isEmpty()) existing = Profile::Current().Path();
    auto dir = QFileDialog::getExistingDirectory(this,
                                                 "Choose Cache Path",
                                                 existing);
    if (!dir.isEmpty()) {
      QSettings().setValue("profileSettings/cachePathOpen", dir);
      cache_path_edit->setText(dir);
    }
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
      existing =
          QSettings().value("profileSettings/userDataPathOpen").toString();
    }
    if (existing.isEmpty()) existing = Profile::Current().Path();
    auto dir = QFileDialog::getExistingDirectory(this,
                                                 "Choose User Data Path",
                                                 existing);
    if (!dir.isEmpty()) {
      QSettings().setValue("profileSettings/userDataPathOpen", dir);
      user_data_path_edit->setText(dir);
    }
  });

  auto curr_settings = profile_.BrowserSettings();
  for (const auto& setting : BrowserSetting::kSettings) {
    settings->AddSettingBreak();

    auto selected = 0;
    if (curr_settings.contains(setting.Key())) {
      selected = curr_settings[setting.Key()] ? 1 : 2;
    }

    auto box = settings->AddComboBoxSetting(
          setting.Name(), setting.Desc(),
          { "Browser Default", "Enabled", "Disabled" },
          selected);
    connect(box, &QComboBox::currentTextChanged, [=]() {
      auto new_settings = profile_.BrowserSettings();
      if (box->currentIndex() == 0) {
        new_settings.remove(setting.Key());
      } else {
        new_settings[setting.Key()] = box->currentIndex() == 1;
      }
      profile_.SetBrowserSettings(new_settings);
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
  for (auto& bubble : temp_bubbles_) {
    auto item = new QListWidgetItem(bubble.Icon(), bubble.FriendlyName());
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (in_use_bubble_ids_.contains(bubble.Id())) {
      item->setText(item->text() + "*");
    }
    list->addItem(item);
  }
  layout->addWidget(list, 1, 0, 1, 5);
  layout->setColumnStretch(0, 1);
  auto in_use_label = new QLabel(
      "* - Bubbles on existing pages cannot be edited or deleted.");
  layout->addWidget(in_use_label, 2, 0, 1, 5);
  if (in_use_bubble_ids_.isEmpty()) in_use_label->setVisible(false);
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
    // Re-index em
    for (int i = 0; i < temp_bubbles_.size(); i++) {
      temp_bubbles_[i].SetOrderIndex(i);
    }
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
      auto editable = !in_use_bubble_ids_.contains(temp_bubbles_[row].Id());
      edit_bubble->setEnabled(editable);
      delete_bubble->setEnabled(editable && list->count() > 1);
    }
  };
  update_buttons();
  connect(list, &QListWidget::itemSelectionChanged, update_buttons);
  connect(list, &QListWidget::itemDoubleClicked, [=](QListWidgetItem* item) {
    auto row = list->row(item);
    if (!in_use_bubble_ids_.contains(temp_bubbles_[row].Id())) {
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
    // Warn if it's in use somewhere
    if (temp_bubbles_[row].Exists() &&
        Workspace::WorkspacePage::BubbleInUse(temp_bubbles_[row].Id())) {
      auto res = QMessageBox::question(
            this,
            "Bubble In Use",
            "This bubble is in use on pages in unopened workspaces. If the "
            "bubble is deleted, the pages will be suspended and set to use "
            "the default bubble when opened next.\nAre you sure you want "
            "to delete this bubble?");
      if (res != QMessageBox::Yes) return;
    }
    delete list->takeItem(row);
    temp_bubbles_.removeAt(row);
    bubbles_changed();
    update_buttons();
  });

  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
}

QWidget* ProfileSettingsDialog::CreateBlockerTab(const Cef& cef) {
  auto layout = new QVBoxLayout;
  layout->addWidget(new QLabel("Blocker Lists:"));

  auto table = new QTableWidget;
  table->setColumnCount(8);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSelectionMode(QAbstractItemView::SingleSelection);
  table->setHorizontalHeaderLabels(
      { "", "Name", "Location", "Homepage", "Version",
        "Expiration", "Last Refreshed", "Rule Count" });
  table->verticalHeader()->setVisible(false);
  table->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
  layout->addWidget(table, 1);

  layout->addWidget(
      new QLabel("Note, upon save, lists regenerate in the background"));

  auto string_item = [](const QString& text,
                        bool editable = false) -> QTableWidgetItem* {
    auto item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (editable) item->setFlags(item->flags() | Qt::ItemIsEditable);
    return item;
  };

  auto add_blocker_list = [=](BlockerList list) {
    temp_blocker_lists_[list.UrlOrLocalPath()] = list;
    auto row = table->rowCount();
    table->setRowCount(row + 1);
    table->setSortingEnabled(false);

    auto item0 = new QTableWidgetItem("");
    item0->setFlags(Qt::ItemIsEnabled |
                    Qt::ItemIsSelectable |
                    Qt::ItemIsUserCheckable);
    item0->setData(Qt::UserRole + 1, list.UrlOrLocalPath());
    auto checked = list.Exists() &&
        profile_.EnabledBlockerListIds().contains(list.Id());
    item0->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    table->setItem(row, 0, item0);
    table->setItem(row, 1, string_item(list.Name(), list.IsLocalOnly()));
    table->setItem(row, 2, string_item(list.UrlOrLocalPath()));
    table->setItem(row, 3, string_item(list.Homepage()));
    table->setItem(row, 4, string_item(
        list.Version() == 0 ? "" : QString::number(list.Version())));
    // TODO(cretz): respect sort
    QString expiration = "never";
    if (list.ExpirationHours() > 0) {
      if (list.ExpirationHours() % 24 == 0) {
        expiration = QString("%1 day(s)").arg(list.ExpirationHours() / 24);
      } else {
        expiration = QString("%1 hour(s)").arg(list.ExpirationHours());
      }
    }
    table->setItem(row, 5, string_item(expiration));
    table->setItem(row, 6, string_item(
        list.LastRefreshed().isNull() ?
            "(pending save)" : list.LastRefreshed().toLocalTime().toString()));
    table->setItem(row, 7, string_item(
        list.LastKnownRuleCount() == 0 ?
            "" : QString::number(list.LastKnownRuleCount())));
    table->setSortingEnabled(true);
  };

  auto button_layout = new QHBoxLayout;

  auto add_url_button = new QToolButton;
  add_url_button->setText("Add From URL");
  button_layout->addWidget(add_url_button);
  connect(add_url_button, &QToolButton::clicked, [=, &cef](bool) {
    AddBlockerListUrl(cef, "");
  });
  try_add_blocker_list_url_ = [=, &cef](const QString& url) {
    AddBlockerListUrl(cef, url);
  };
  connect(this, &ProfileSettingsDialog::BlockerUrlLoadComplete,
          [=](BlockerList list, bool ok) {
    if (ok) {
      add_blocker_list(list);
      emit Changed();
    }
  });

  auto add_local_button = new QToolButton;
  add_local_button->setText("Add From File");
  button_layout->addWidget(add_local_button);
  connect(add_local_button, &QToolButton::clicked, [=](bool) {
    QString dir = QSettings().value(
          "profileSettings/addBlockerLocalOpen").toString();
    auto file = QFileDialog::getOpenFileName(this, "Add From File", dir);
    if (file.isEmpty()) return;
    file = QDir::toNativeSeparators(file);
    QSettings().setValue("profileSettings/addBlockerLocalOpen",
                         QFileInfo(file).dir().path());
    if (temp_blocker_lists_.contains(file)) {
      QMessageBox::warning(this, "Add Blocker List", "File already exists.");
      QTimer::singleShot(0, [=]() { add_local_button->click(); });
      return;
    }
    auto ok = false;
    auto list = BlockerList::FromFile(file, &ok);
    if (!ok) {
      QMessageBox::warning(this, "Add Blocker List", "Failed to load list.");
      QTimer::singleShot(0, [=]() { add_local_button->click(); });
      return;
    }
    add_blocker_list(list);
    emit Changed();
  });

  button_layout->addStretch(1);

  auto selected_row = [=]() -> int {
    auto rows = table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
  };

  auto refresh_button = new QToolButton;
  refresh_button->setText("Force Reload on Save");
  refresh_button->setEnabled(false);
  button_layout->addWidget(refresh_button);
  connect(refresh_button, &QToolButton::clicked, [=](bool) {
    auto row = selected_row();
    if (row == -1) return;
    auto key = table->item(row, 0)->data(Qt::UserRole + 1).toString();
    blocker_list_ids_pending_refresh_ << temp_blocker_lists_.value(key).Id();
    table->setItem(row, 5, string_item("(pending save)"));
    refresh_button->setEnabled(false);
    emit Changed();
  });

  auto delete_button = new QToolButton;
  delete_button->setText("Delete");
  delete_button->setEnabled(false);
  button_layout->addWidget(delete_button);
  connect(delete_button, &QToolButton::clicked, [=](bool) {
    auto row = selected_row();
    if (row == -1) return;
    auto key = table->item(row, 0)->data(Qt::UserRole + 1).toString();
    temp_blocker_lists_.remove(key);
    enabled_blocker_lists_.remove(key);
    table->removeRow(row);
    emit Changed();
  });

  layout->addLayout(button_layout);

  connect(table, &QTableWidget::itemSelectionChanged, [=]() {
    refresh_button->setEnabled(false);
    delete_button->setEnabled(false);
    auto row = selected_row();
    if (row == -1) return;
    delete_button->setEnabled(true);
    auto key = table->item(row, 0)->data(Qt::UserRole + 1).toString();
    auto list = temp_blocker_lists_.value(key);
    if (list.Exists() &&
        !blocker_list_ids_pending_refresh_.contains(list.Id())) {
      refresh_button->setEnabled(true);
    }
  });

  connect(table, &QTableWidget::itemChanged, [=](QTableWidgetItem* item) {
    if (table->column(item) != 0) return;
    auto key = item->data(Qt::UserRole + 1).toString();
    // Change the check state maybe
    if (item->checkState() == Qt::Checked) {
      enabled_blocker_lists_.insert(key);
    } else {
      enabled_blocker_lists_.remove(key);
    }
    emit Changed();
  });

  orig_blocker_lists_ = BlockerList::Lists();
  for (const auto& orig_list : orig_blocker_lists_) {
    add_blocker_list(orig_list);
    if (profile_.EnabledBlockerListIds().contains(orig_list.Id())) {
      enabled_blocker_lists_ << orig_list.UrlOrLocalPath();
    }
  }

  auto widg = new QWidget;
  widg->setLayout(layout);
  return widg;
}

void ProfileSettingsDialog::AddBlockerListUrl(const Cef& cef,
                                              const QString& default_url) {
  BlockerList empty;
  auto url = QInputDialog::getText(
      this, "Add Blocker List From URL", "List URL:",
      QLineEdit::Normal, default_url, nullptr, Qt::WindowFlags(),
      Qt::ImhUrlCharactersOnly);
  if (url.isEmpty()) {
    emit BlockerUrlLoadComplete(empty, false);
    return;
  }
  if (temp_blocker_lists_.contains(url)) {
    QMessageBox::warning(this, "Add Blocker List", "URL already exists.");
    emit BlockerUrlLoadComplete(empty, false);
    return;
  }

  // Open the progress dialog. We do this in a pointer so we can know
  //  when it's deleted in the callback
  QPointer<QProgressDialog> progress_dialog =
      new QProgressDialog("Loading List", "Cancel Load", 0, 0, this);
  progress_dialog->setWindowModality(Qt::WindowModal);
  progress_dialog->setMinimumDuration(0);
  // Resulting list, holds one or none
  QList<BlockerList> done_list;
  auto cancel = BlockerList::FromUrl(
        cef, url, [=, &done_list](BlockerList list, bool ok) {
    // We only add this to the done list if the progress dialog is still around
    if (progress_dialog) {
      if (ok) done_list << list;
      progress_dialog->done(ok ? QDialog::Accepted : QDialog::Rejected);
    }
  });
  connect(progress_dialog, &QProgressDialog::canceled, cancel);
  auto result = progress_dialog->exec();
  // Delete the dialog right away so the callback can't see it
  delete progress_dialog;
  if (result == QDialog::Rejected || done_list.isEmpty()) {
    QMessageBox::warning(nullptr, "Add Blocker List", "Failed to load list.");
    emit BlockerUrlLoadComplete(empty, false);
  } else if (temp_blocker_lists_.contains(done_list.first().Url())) {
    QMessageBox::warning(nullptr, "Add Blocker List", "URL already exists.");
    emit BlockerUrlLoadComplete(empty, false);
  } else {
    emit BlockerUrlLoadComplete(done_list.first(), true);
  }
}

}  // namespace doogie
