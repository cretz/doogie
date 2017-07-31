#include "profile_change_dialog.h"
#include "profile.h"

namespace doogie {

ProfileChangeDialog::ProfileChangeDialog(QWidget* parent) : QDialog(parent) {
  auto profile = Profile::Current();
  auto layout = new QGridLayout;
  layout->addWidget(new QLabel("Current Profile:"), 0, 0);
  layout->addWidget(new QLabel(profile->FriendlyName()), 0, 1);
  layout->addWidget(new QLabel("New/Existing Profile:"), 1, 0);

  selector_ = new QComboBox;
  selector_->setEditable(true);
  selector_->setInsertPolicy(QComboBox::NoInsert);
  // Show all last ten but the current
  auto found_in_mem = false;
  for (auto const& other_path : Profile::LastTenProfilePaths()) {
    if (other_path != profile->Path()) {
      if (profile->InMemory()) found_in_mem = true;
      selector_->addItem(Profile::FriendlyName(other_path), other_path);
    }
  }
  if (!found_in_mem && !profile->InMemory()) {
    selector_->addItem(Profile::FriendlyName(Profile::kInMemoryPath),
                      Profile::kInMemoryPath);
  }
  selector_->addItem("<choose profile folder...>");
  selector_->lineEdit()->setPlaceholderText("Get or Create Profile...");
  selector_->setCurrentIndex(-1);
  connect(selector_, static_cast<void(QComboBox::*)(int)>(
            &QComboBox::currentIndexChanged), [this, profile](int index) {
    // If they clicked the last index, we pop open a file dialog
    if (index == selector_->count() - 1) {
      auto open_dir = QDir(profile->Path());
      open_dir.cdUp();
      auto path = QFileDialog::getExistingDirectory(
            nullptr, "Choose Profile Directory", open_dir.path());
      if (!path.isEmpty()) {
        selector_->setCurrentText(
              Profile::FriendlyName(QDir::toNativeSeparators(path)));
      } else {
        selector_->clearEditText();
        selector_->setCurrentIndex(-1);
      }
    }
  });
  layout->addWidget(selector_, 1, 1);
  layout->setColumnStretch(1, 1);

  auto buttons = new QHBoxLayout;
  auto launch = new QPushButton("Launch New Doogie With Profile");
  launch->setEnabled(false);
  launch->setDefault(true);
  auto restart = new QPushButton("Restart Doogie With Profile");
  restart->setEnabled(false);
  auto cancel = new QPushButton("Cancel");
  connect(selector_, &QComboBox::editTextChanged,
          [this, restart, launch](const QString& text) {
    restart->setEnabled(!text.isEmpty());
    launch->setEnabled(!text.isEmpty());
  });
  connect(launch, &QPushButton::clicked, [this]() {
    needs_restart_ = false;
    accept();
  });
  connect(restart, &QPushButton::clicked, [this]() {
    needs_restart_ = true;
    accept();
  });
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
  buttons->addWidget(launch);
  buttons->addWidget(restart);
  buttons->addWidget(cancel);
  layout->addLayout(buttons, 2, 0, 1, 2, Qt::AlignRight);

  setWindowTitle("Change Profile");
  setLayout(layout);
}

bool ProfileChangeDialog::NeedsRestart() {
  return needs_restart_;
}

QString ProfileChangeDialog::ChosenPath() {
  if (selector_->currentData().isValid()) {
    return selector_->currentData().toString();
  }
  return selector_->currentText();
}

void ProfileChangeDialog::done(int r) {
  if (r == Accepted) {
    auto path = ChosenPath();
    if (!Profile::CreateOrLoadProfile(path, false)) {
      QMessageBox::critical(
            nullptr, "Change Profile",
            QString("Unable to change profile to dir: ") + path);
      return;
    }
  }
  QDialog::done(r);
}

}  // namespace doogie
