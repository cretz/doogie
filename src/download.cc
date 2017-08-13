#include "download.h"

namespace doogie {

Download::Download(CefRefPtr<CefDownloadItem> item,
                   CefRefPtr<CefDownloadItemCallback> update_callback)
    : update_callback_(update_callback) {
  id_ = item->GetId();
  mime_type_ = QString::fromStdString(item->GetMimeType().ToString());
  orig_url_ = QString::fromStdString(item->GetOriginalUrl().ToString());
  url_ = QString::fromStdString(item->GetURL().ToString());
  suggested_file_name_ =
      QString::fromStdString(item->GetSuggestedFileName().ToString());
  path_ = QString::fromStdString(item->GetFullPath().ToString());
  current_state_ = item->IsCanceled() ? Download::Canceled :
      (item->IsComplete() ? Download::Complete :
          (item->IsInProgress() ? Download::InProgress : Download::Unknown));
  start_time_ = QDateTime::fromSecsSinceEpoch(item->GetStartTime().GetTimeT(),
                                              Qt::UTC);
  end_time_ = QDateTime::fromSecsSinceEpoch(item->GetEndTime().GetTimeT(),
                                            Qt::UTC);
  bytes_received_ = item->GetReceivedBytes();
  total_bytes_ = item->GetTotalBytes();
  current_bytes_per_sec_ = item->GetCurrentSpeed();
}

void Download::Cancel() {
  if (update_callback_) update_callback_->Cancel();
}

void Download::Pause() {
  if (update_callback_) update_callback_->Pause();
}

void Download::Resume() {
  if (update_callback_) update_callback_->Resume();
}

}  // namespace doogie
