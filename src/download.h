#ifndef DOOGIE_DOWNLOAD_H_
#define DOOGIE_DOWNLOAD_H_

#include <QtSql>
#include <QtWidgets>

#include "cef/cef.h"

namespace doogie {

// DB model representing a known download.
class Download {
 public:
  enum State {
    // Note, this is the value even when paused
    InProgress,
    Canceled,
    Complete,
    Unknown
  };

  // Oldest first
  static QList<Download> Downloads();
  static bool ClearDownloads(QList<qlonglong> exclude_ids);

  Download();
  explicit Download(
      CefRefPtr<CefDownloadItem> item,
      CefRefPtr<CefDownloadItemCallback> update_callback = nullptr);
  explicit Download(
      CefRefPtr<CefDownloadItem> item,
      const QString& suggested_file_name);

  void Cancel();
  void Pause();
  void Resume();

  // Note, if already exists, only the end time and state are
  //  updated (when end time is present)
  bool Persist();
  bool Delete();

  qlonglong DbId() const { return db_id_; }
  void SetDbId(qlonglong db_id) { db_id_ = db_id; }
  bool Exists() const { return db_id_ > -1; }

  uint LiveId() const { return live_id_; }
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
  explicit Download(const QSqlRecord& record);

  void FromCef(CefRefPtr<CefDownloadItem> item);

  qlonglong db_id_ = -1;
  uint live_id_ = -1;
  QString mime_type_;
  QString orig_url_;
  QString url_;
  QString suggested_file_name_;
  QString path_;

  State current_state_ = Unknown;
  QDateTime start_time_;
  QDateTime end_time_;
  qint64 bytes_received_ = -1;
  qint64 total_bytes_ = -1;
  qint64 current_bytes_per_sec_ = -1;

  CefRefPtr<CefDownloadItemCallback> update_callback_;
};

}  // namespace doogie

#endif  // DOOGIE_DOWNLOAD_H_
