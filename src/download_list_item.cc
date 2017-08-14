#include "download_list_item.h"

#include "util.h"

namespace doogie {

DownloadListItem::DownloadListItem() : QListWidgetItem() {
}

void DownloadListItem::AfterAdded() {
  // Create the widget to show download info
  auto layout = new QHBoxLayout;

  auto label_layout = new QVBoxLayout;
  name_label_ = new QLabel;
  label_layout->addWidget(name_label_);
  progress_bar_ = new QProgressBar;
  label_layout->addWidget(progress_bar_);
  sub_name_label_ = new QLabel;
  sub_name_label_->setStyleSheet("color: gray;");
  label_layout->addWidget(sub_name_label_);
  layout->addLayout(label_layout, 1);

  auto button_layout = new QHBoxLayout;

  pause_button_ = new QToolButton;
  pause_button_->setIcon(QIcon(":/res/images/fontawesome/pause.png"));
  pause_button_->setToolTip("Pause Download");
  pause_button_->setAutoRaise(true);
  button_layout->addWidget(pause_button_);
  listWidget()->connect(pause_button_, &QToolButton::clicked, [=]() {
    download_.Pause();
  });

  resume_button_ = new QToolButton;
  resume_button_->setIcon(QIcon(":/res/images/fontawesome/play.png"));
  resume_button_->setToolTip("Resume Download");
  resume_button_->setAutoRaise(true);
  button_layout->addWidget(resume_button_);
  listWidget()->connect(resume_button_, &QToolButton::clicked, [=]() {
    download_.Resume();
  });

  cancel_button_ = new QToolButton;
  cancel_button_->setIcon(QIcon(":/res/images/fontawesome/times.png"));
  cancel_button_->setToolTip("Cancel Download");
  cancel_button_->setAutoRaise(true);
  button_layout->addWidget(cancel_button_);
  listWidget()->connect(cancel_button_, &QToolButton::clicked, [=]() {
    download_.Cancel();
  });

  open_folder_button_ = new QToolButton;
  open_folder_button_->setIcon(
      QIcon(":/res/images/fontawesome/folder-open-o.png"));
  open_folder_button_->setToolTip("Open Containing Folder");
  open_folder_button_->setAutoRaise(true);
  button_layout->addWidget(open_folder_button_);
  listWidget()->connect(open_folder_button_, &QToolButton::clicked, [=]() {
    Util::OpenContainingFolder(download_.Path());
  });

  layout->addLayout(button_layout);

  auto widg = new QWidget;
  widg->setLayout(layout);
  listWidget()->setItemWidget(this, widg);
}

bool DownloadListItem::UpdateDownload(const Download& d) {
  // We only update live, and it has to be the right live ID
  if (download_.LiveId() != download_.LiveId()) return false;

  // Set it and persist it
  auto old_id = download_.DbId();
  download_ = d;
  download_.SetDbId(old_id);
  download_.Persist();

  // Update the widgets
  QFileInfo file(download_.Path());
  name_label_->setText(file.fileName());
  QStringList sub_pieces;
  if (download_.CurrentState() == Download::InProgress ||
      download_.CurrentState() == Download::Paused) {
    if (download_.CurrentState() == Download::Paused) {
      sub_pieces << "Paused";
    }
    if (download_.TotalBytes() > 0) {
      // curr/max is over 10000 steps
      if (progress_bar_->maximum() == 0) {
        progress_bar_->setMaximum(download_.TotalBytes() / 10000);
      }
      progress_bar_->setValue(download_.BytesReceived() / 10000);
      if (download_.CurrentBytesPerSec() > 0) {
        auto seconds_remaining =
          (download_.TotalBytes() - download_.BytesReceived()) /
            download_.CurrentBytesPerSec();
        sub_pieces << Util::FriendlyTimeSpan(seconds_remaining) + " left";
      }
      sub_pieces << Util::FriendlyByteSize(download_.BytesReceived()) +
                    " of " + Util::FriendlyByteSize(download_.TotalBytes());
    } else {
      sub_pieces << Util::FriendlyByteSize(download_.BytesReceived()) +
                    " so far";
    }
    if (download_.CurrentBytesPerSec() > 0) {
      sub_pieces << Util::FriendlyByteSize(download_.CurrentBytesPerSec()) +
                    "/sec";
    }
    pause_button_->setVisible(
        download_.CurrentState() == Download::InProgress);
    resume_button_->setVisible(download_.CurrentState() == Download::Paused);
    cancel_button_->setVisible(true);
    open_folder_button_->setVisible(false);
  } else {
    progress_bar_->hide();
    pause_button_->setVisible(false);
    resume_button_->setVisible(false);
    cancel_button_->setVisible(false);
    open_folder_button_->setVisible(false);
    if (download_.CurrentState() == Download::Canceled) {
      sub_pieces << "Canceled";
    } else if (file.exists()) {
      open_folder_button_->setVisible(true);
      sub_pieces << Util::FriendlyByteSize(file.size());
    } else {
      sub_pieces << "File not found";
    }
    QUrl url(download_.OrigUrl(), QUrl::StrictMode);
    if (url.isValid()) sub_pieces << url.host();
    auto time = download_.EndTime().isValid() ?
        download_.EndTime() : download_.StartTime();
    sub_pieces << time.toLocalTime().toString();
  }
  sub_name_label_->setText(sub_pieces.join(" - "));
}

void DownloadListItem::ApplyContextMenu(QMenu* menu) {
  auto from_button = [=](QToolButton* button) {
    menu->addAction(button->toolTip(), button, &QToolButton::click)->
        setEnabled(button->isVisible());
  };

  from_button(pause_button_);
  from_button(resume_button_);
  from_button(cancel_button_);
  from_button(open_folder_button_);
  menu->addAction("Copy Download Link to Clipboard", [=]() {
    QGuiApplication::clipboard()->setText(download_.Url());
  });
  menu->addAction("Remove Download from List", [=]() {
    DeleteAndRemove();
  });
}

void DownloadListItem::DeleteAndRemove() {
  // Ignore DB delete result for now
  download_.Delete();
  delete this;
}

}  // namespace doogie
