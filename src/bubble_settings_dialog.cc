#include "bubble_settings_dialog.h"

#include "browser_setting.h"
#include "profile.h"
#include "settings_widget.h"
#include "util.h"

namespace doogie {

qlonglong BubbleSettingsDialog::NewBubble(QWidget* parent) {
  QSet<QString> invalid_names;
  for (auto& bubble : Bubble::CachedBubbles()) {
    invalid_names.insert(bubble.Name());
  }
  Bubble bubble;
  if (!UpdateBubble(&bubble, invalid_names, parent)) {
    return -1;
  }
  // Save it and clear the cache
  bubble.SetOrderIndex(Bubble::CachedBubbles().size());
  if (!bubble.Persist()) {
    qCritical() << "Failed to save current profile after bubble add";
    return -1;
  }

  Bubble::InvalidateCachedBubbles();

  return bubble.Id();
}

bool BubbleSettingsDialog::UpdateBubble(Bubble* bubble,
                                        QSet<QString> invalid_names,
                                        QWidget* parent) {
  return BubbleSettingsDialog(bubble,
                              invalid_names,
                              parent).exec() == QDialog::Accepted;
}

BubbleSettingsDialog::BubbleSettingsDialog(Bubble* bubble,
                                           QSet<QString> invalid_names,
                                           QWidget* parent)
    : QDialog(parent), orig_bubble_(bubble), invalid_names_(invalid_names) {
  bubble_.CopySettingsFrom(*bubble);
  auto ok = new QPushButton("OK");
  ok->setEnabled(false);
  auto cancel = new QPushButton("Cancel");

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
  layout->addWidget(ok, 6, 0, Qt::AlignRight);
  layout->addWidget(cancel, 6, 1);

  setWindowTitle("Bubble Settings");
  setSizeGripEnabled(true);
  setLayout(layout);

  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

  connect(this, &BubbleSettingsDialog::Changed, [=]() {
    ok->setEnabled(*orig_bubble_ != bubble_ &&
        !invalid_names_.contains(bubble_.Name()));
  });
  emit Changed();

  restoreGeometry(QSettings().value("bubbleSettings/geom").toByteArray());
}

void BubbleSettingsDialog::done(int r) {
  if (r == Accepted) {
    orig_bubble_->CopySettingsFrom(bubble_);
  }
  QDialog::done(r);
}

void BubbleSettingsDialog::closeEvent(QCloseEvent* event) {
  QSettings().setValue("bubbleSettings/geom", saveGeometry());
  QDialog::closeEvent(event);
}

void BubbleSettingsDialog::keyPressEvent(QKeyEvent* event) {
  // Don't let escape close this if there are changes
  if (event->key() == Qt::Key_Escape && *orig_bubble_ != bubble_) {
    event->ignore();
    return;
  }
  QDialog::keyPressEvent(event);
}

QLayoutItem* BubbleSettingsDialog::CreateNameSection() {
  auto layout = new QHBoxLayout;
  layout->setSpacing(5);
  layout->addWidget(new QLabel("Name:"));
  auto name_edit = new QLineEdit;
  name_edit->setPlaceholderText("(default)");
  name_edit->setText(bubble_.Name());
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
    bubble_.SetName(name_edit->text());
    emit Changed();
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
    icon_pixmap->clear();
    icon_label->clear();
    icon_color_enabled->setChecked(false);
    icon_color->setIcon(QIcon());
    icon_color->setText("");
    if (!bubble_.IconPath().isNull()) {
      icon_label->setEnabled(true);
      icon_enabled->setChecked(true);
      auto icon = bubble_.Icon();
      if (!icon.isNull()) icon_pixmap->setPixmap(icon.pixmap(16, 16));
      auto text = bubble_.IconPath();
      if (text.isEmpty()) {
        icon_label->setText("(default)");
      } else {
        icon_label->setText(text);
      }
      choose_file->setEnabled(true);
      reset->setEnabled(true);
      icon_color_enabled->setEnabled(true);
      icon_color->setEnabled(true);
      if (bubble_.IconColor().isValid()) {
        icon_color_enabled->setChecked(true);
        QPixmap pixmap(16, 16);
        pixmap.fill(bubble_.IconColor());
        icon_color->setIcon(QIcon(pixmap));
        icon_color->setText(bubble_.IconColor().name());
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
    emit Changed();
  };
  update_icon_info();
  connect(icon_enabled, &QCheckBox::clicked, [=](bool checked) {
    if (checked) {
      bubble_.SetIconPath("");
    } else {
      bubble_.SetIconPath(QString());
    }
    update_icon_info();
  });
  connect(choose_file, &QPushButton::clicked, [=]() {
    auto filter = "Images (*.bmp *.gif *.jpg *.jpeg *.png "
                  "*.pbm *.pgm *.ppm *.xbm *.xpm *.svg)";
    auto existing_dir =
        QSettings().value("bubbleSettings/iconFileOpen").toString();
    if (!bubble_.IconPath().isEmpty()) {
      existing_dir = QFileInfo(bubble_.IconPath()).dir().path();
    }
    if (existing_dir.isEmpty()) {
      existing_dir = QStandardPaths::writableLocation(
          QStandardPaths::PicturesLocation);
    }
    auto file = QFileDialog::getOpenFileName(this, "Choose Image File",
                                             existing_dir, filter);
    if (!file.isEmpty() && !QImageReader::imageFormat(file).isEmpty()) {
      bubble_.SetIconPath(file);
      update_icon_info();
      QSettings().setValue("bubbleSettings/iconFileOpen",
                           QFileInfo(file).dir().path());
    }
  });
  connect(reset, &QPushButton::clicked, [=]() {
    bubble_.SetIconPath("");
    update_icon_info();
  });
  connect(icon_color_enabled, &QCheckBox::clicked, [=](bool checked) {
    if (checked) {
      bubble_.SetIconColor(QColor("black"));
    } else {
      bubble_.SetIconColor(QColor());
    }
    update_icon_info();
  });
  connect(icon_color, &QToolButton::clicked, [=]() {
    auto color = QColorDialog::getColor(bubble_.IconColor(),
                                        this,
                                        "Bubble Color Override");
    if (color.isValid()) {
      bubble_.SetIconColor(color);
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

  auto cache_path_layout = new QHBoxLayout;
  auto cache_path_disabled = new QCheckBox("No cache");
  auto cache_path = bubble_.CachePath();
  cache_path_disabled->setChecked(!cache_path.isNull() && cache_path.isEmpty());
  cache_path_layout->addWidget(cache_path_disabled);
  auto cache_path_edit = new QLineEdit;
  cache_path_edit->setPlaceholderText("Same as profile");
  cache_path_edit->setText(cache_path);
  cache_path_edit->setEnabled(!cache_path_disabled->isChecked());
  cache_path_layout->addWidget(cache_path_edit, 1);
  auto cache_path_open = new QToolButton();
  cache_path_open->setAutoRaise(true);
  cache_path_open->setText("...");
  cache_path_open->setEnabled(!cache_path_disabled->isChecked());
  cache_path_layout->addWidget(cache_path_open);
  auto cache_path_widg = new QWidget;
  cache_path_widg->setLayout(cache_path_layout);

  auto cache_path_update = [=]() {
    cache_path_edit->setEnabled(!cache_path_disabled->isChecked());
    cache_path_open->setEnabled(!cache_path_disabled->isChecked());

    if (cache_path_disabled->isChecked()) {
      bubble_.SetCachePath("");
    } else {
      auto text = cache_path_edit->text();
      // Empty means null for us
      bubble_.SetCachePath(text.isEmpty() ? QString() : text);
    }
    emit Changed();
  };
  connect(cache_path_disabled, &QCheckBox::toggled, cache_path_update);
  connect(cache_path_edit, &QLineEdit::textChanged, cache_path_update);
  connect(cache_path_open, &QToolButton::clicked, [=]() {
    auto existing = cache_path_edit->text();
    if (existing.isEmpty()) {
      existing = QSettings().value("bubbleSettings/cachePathOpen").toString();
    }
    if (existing.isEmpty()) existing = Profile::Current().Path();
    auto dir = QFileDialog::getExistingDirectory(this,
                                                 "Choose Cache Path",
                                                 existing);
    if (!dir.isEmpty()) {
      QSettings().setValue("bubbleSettings/cachePathOpen", dir);
      cache_path_edit->setText(dir);
    }
  });
  settings->AddSetting(
        "Cache Path",
        "The location where cache data is stored on disk, if any. ",
        cache_path_widg);
  settings->AddSettingBreak();

  auto curr_settings = bubble_.BrowserSettings();
  for (auto& setting : BrowserSetting::kSettings) {
    settings->AddSettingBreak();

    auto selected = 0;
    if (curr_settings.contains(setting.Key())) {
      selected = curr_settings[setting.Key()] ? 1 : 2;
    }

    auto box = settings->AddComboBoxSetting(
          setting.Name(), setting.Desc(),
          { "Same as profile", "Enabled", "Disabled" },
          selected);
    connect(box, &QComboBox::currentTextChanged, [=]() {
      auto new_settings = bubble_.BrowserSettings();
      if (box->currentIndex() == 0) {
        new_settings.remove(setting.Key());
      } else {
        new_settings[setting.Key()] = box->currentIndex() == 1;
      }
      bubble_.SetBrowserSettings(new_settings);
      emit Changed();
    });
  }

  return layout;
}

}  // namespace doogie
