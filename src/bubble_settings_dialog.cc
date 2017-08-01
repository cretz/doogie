#include "bubble_settings_dialog.h"

namespace doogie {

BubbleSettingsDialog::BubbleSettingsDialog(Bubble* bubble,
                                           QStringList invalid_names,
                                           QWidget* parent)
    : QDialog(parent), bubble_(bubble), invalid_names_(invalid_names) {
  ok_ = new QPushButton("OK");
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
  layout->setColumnStretch(0, 1);
  layout->addWidget(ok_, 3, 0, Qt::AlignRight);
  layout->addWidget(cancel_, 3, 1);

  setWindowTitle("Profile Settings");
  setSizeGripEnabled(true);
  setLayout(layout);
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
  connect(name_edit, &QLineEdit::textChanged, [=]() {
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
  });
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
  hlayout->addStretch(1);
  auto choose_file = new QPushButton("Choose Image File");
  hlayout->addWidget(choose_file);
  auto reset = new QPushButton("Reset to Default");
  hlayout->addWidget(reset);
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
    auto existing_dir = QStandardPaths::writableLocation(
          QStandardPaths::PicturesLocation);
    auto icon_path = bubble_->prefs_.value("icon").toString();
    if (!icon_path.isEmpty()) {
      existing_dir = QFileInfo(icon_path).dir().path();
    }
    auto file = QFileDialog::getOpenFileName(this, "Choose Image File",
                                             existing_dir, filter);
    if (!file.isEmpty() && !QImageReader::imageFormat(file).isEmpty()) {
      bubble_->prefs_["icon"] = file;
      update_icon_info();
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

}  // namespace doogie
