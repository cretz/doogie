#ifndef DOOGIE_DOWNLOAD_H_
#define DOOGIE_DOWNLOAD_H_

#include <QtWidgets>

#include "cef/cef.h"

namespace doogie {

class Download {
 public:
  enum State {
    InProgress,
    Paused,
    Canceled,
    Complete,
    Unknown
  };

  explicit Download(
      CefRefPtr<CefDownloadItem> item,
      CefRefPtr<CefDownloadItemCallback> update_callback = nullptr);

  void Cancel();
  void Pause();
  void Resume();

  uint Id() const { return id_; }
  QString MimeType() const { return mime_type_; }
  QString OrigUrl() const { return orig_url_; }
  QString Url() const { return url_; }
  QString SuggestedFileName() const { return suggested_file_name_; }
  QString Path() const { return path_; }

  State CurrentState() const { return current_state_; }
  QDateTime StartTime() const { return start_time_; }
  QDateTime EndTime() const { return end_time_; }
  qint64 BytesReceived() const { return bytes_received_; }
  qint64 TotalBytes() const { return total_bytes_; }
  qint64 CurrentBytesPerSec() const { return current_bytes_per_sec_; }

 private:
  uint id_;
  QString mime_type_;
  QString orig_url_;
  QString url_;
  QString suggested_file_name_;
  QString path_;

  State current_state_;
  QDateTime start_time_;
  QDateTime end_time_;
  qint64 bytes_received_;
  qint64 total_bytes_;
  qint64 current_bytes_per_sec_;

  CefRefPtr<CefDownloadItemCallback> update_callback_;
};

}  // namespace doogie

#endif  // DOOGIE_DOWNLOAD_H_
