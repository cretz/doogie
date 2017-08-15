#include "download_list_item.h"

#include "util.h"

namespace doogie {

DownloadListItem::DownloadListItem() : QListWidgetItem() {
}

void DownloadListItem::AfterAdded() {
  // Create the widget to show download info
  auto layout = new QGridLayout;
  layout->setSpacing(1);

  name_label_ = new QLabel;
  layout->addWidget(name_label_, 0, 0);
  layout->setColumnStretch(0, 1);
  progress_bar_ = new QProgressBar;
  progress_bar_->setMaximumHeight(10);
  progress_bar_->setMinimum(0);
  layout->addWidget(progress_bar_, 1, 0);

  auto button_layout = new QHBoxLayout;

  pause_button_ = new QToolButton;
  pause_button_->setIcon(QIcon(":/res/images/fontawesome/pause.png"));
  pause_button_->setToolTip("Pause Download");
  pause_button_->setAutoRaise(true);
  button_layout->addWidget(pause_button_);

  resume_button_ = new QToolButton;
  resume_button_->setVisible(false);
  resume_button_->setIcon(QIcon(":/res/images/fontawesome/play.png"));
  resume_button_->setToolTip("Resume Download");
  resume_button_->setAutoRaise(true);
  button_layout->addWidget(resume_button_);
  listWidget()->connect(pause_button_, &QToolButton::clicked, [=]() {
    pause_button_->setVisible(false);
    resume_button_->setVisible(true);
    download_.Pause();
  });
  listWidget()->connect(resume_button_, &QToolButton::clicked, [=]() {
    pause_button_->setVisible(true);
    resume_button_->setVisible(false);
    download_.Resume();
  });

  cancel_button_ = new QToolButton;
  cancel_button_->setIcon(QIcon(":/res/images/fontawesome/times.png"));
  cancel_button_->setToolTip("Cancel Download");
  cancel_button_->setAutoRaise(true);
  button_layout->addWidget(cancel_button_);
  listWidget()->connect(cancel_button_, &QToolButton::clicked, [=]() {
    download_.Cancel();
    // Since the path changes on this to nothing because of a cancel
    //  the update will never get called back, so we call it.
    UpdateDownload(download_);
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

  layout->addLayout(button_layout, 0, 1, 2, 1);

  sub_name_label_ = new QLabel;
  sub_name_label_->setStyleSheet("color: gray;");
  layout->addWidget(sub_name_label_, 2, 0, 1, 2, Qt::AlignTop);

  auto widg = new QWidget;
  widg->setLayout(layout);
  widg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  setSizeHint(widg->sizeHint());
  listWidget()->setItemWidget(this, widg);
}

bool DownloadListItem::UpdateDownload(const Download& d) {
  // We only update live, and it has to be the right live ID
  if (download_.Exists() && download_.LiveId() != d.LiveId()) return false;

  // Set it and persist it
  auto old_id = -1;
  if (download_.Exists()) old_id = download_.DbId();
  download_ = d;
  if (old_id >= 0) download_.SetDbId(old_id);
  download_.Persist();

  // Update the widgets
  QFileInfo file(download_.Path());
  name_label_->setText(file.fileName());
  QStringList sub_pieces;
  if (download_.CurrentState() == Download::InProgress) {
    auto paused = pause_button_->isHidden();
    if (paused) sub_pieces << "Paused";
    // Byte steps over 10000
    int recv_steps = download_.BytesReceived() / 10000;
    int total_steps = (download_.TotalBytes() / 10000) + 1;
    if (download_.TotalBytes() > 0) {
      progress_bar_->setMaximum(total_steps);
      progress_bar_->setValue(recv_steps);
      if (!paused && download_.CurrentBytesPerSec() > 0) {
        auto seconds_remaining =
          (download_.TotalBytes() - download_.BytesReceived()) /
            download_.CurrentBytesPerSec();
        sub_pieces << Util::FriendlyTimeSpan(seconds_remaining + 1) + " left";
      }
      sub_pieces << Util::FriendlyByteSize(download_.BytesReceived()) +
                    " of " + Util::FriendlyByteSize(download_.TotalBytes());
    } else {
      sub_pieces << Util::FriendlyByteSize(download_.BytesReceived()) +
                    " so far";
    }
    if (!paused && download_.CurrentBytesPerSec() > 0) {
      sub_pieces << Util::FriendlyByteSize(download_.CurrentBytesPerSec()) +
                    "/sec";
    }
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
  return true;
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
    QGuiApplication::clipboard()->setText(download_.OrigUrl());
  });
  menu->addAction("Remove Finished Download", [=]() {
    DeleteAndRemoveIfDone();
  })->setEnabled(download_.CurrentState() == Download::Complete ||
                 download_.CurrentState() == Download::Canceled);
}

void DownloadListItem::DeleteAndRemoveIfDone() {
  if (download_.CurrentState() == Download::Complete ||
      download_.CurrentState() == Download::Canceled) {
    // Ignore DB delete result for now
    download_.Delete();
    delete this;
  }
}

bool DownloadListItem::DownloadActive() {
  return download_.CurrentState() == Download::InProgress;
}

}  // namespace doogie
