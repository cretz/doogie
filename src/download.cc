#include "download.h"

#include "sql.h"

namespace doogie {

QList<Download> Download::Downloads() {
  QList<Download> ret;
  QSqlQuery query;
  if (!Sql::Exec(&query, "SELECT * FROM download ORDER BY start_time")) {
    return ret;
  }
  while (query.next()) ret.append(Download(query.record()));
  return ret;
}

bool Download::ClearDownloads(QList<qlonglong> exclude_ids) {
  QString sql = "DELETE FROM download";
  if (!exclude_ids.isEmpty()) {
    sql += " WHERE id NOT IN (";
    for (int i = 0; i < exclude_ids.size(); i++) {
      if (i > 0) sql += ",";
      sql += QString::number(exclude_ids[i]);
    }
    sql += ")";
  }
  QSqlQuery query;
  return Sql::Exec(&query, sql);
}

Download::Download() {}

Download::Download(CefRefPtr<CefDownloadItem> item,
                   CefRefPtr<CefDownloadItemCallback> update_callback)
    : update_callback_(update_callback) {
  FromCef(item);
}

Download::Download(CefRefPtr<CefDownloadItem> item,
                   const QString& suggested_file_name) {
  FromCef(item);
  suggested_file_name_ = suggested_file_name;
}

void Download::Cancel() {
  if (update_callback_) update_callback_->Cancel();
  current_state_ = Canceled;
}

void Download::Pause() {
  if (update_callback_) update_callback_->Pause();
}

void Download::Resume() {
  if (update_callback_) update_callback_->Resume();
}

bool Download::Persist() {
  QSqlQuery query;
  if (Exists()) {
    if (end_time_.isNull()) return true;
    return Sql::ExecParam(
          &query,
          "UPDATE download SET end_time = ?, success = ? "
          "WHERE id = ?",
          { end_time_.toSecsSinceEpoch(),
            current_state_ == Complete, db_id_ });
  }
  auto ok = Sql::ExecParam(
        &query,
        "INSERT INTO download ( "
        "  mime_type, orig_url, url, path, "
        "  success, start_time, size "
        ") VALUES (?, ?, ?, ?, ?, ?, ?)",
        { mime_type_, orig_url_, url_, path_,
          current_state_ == Complete,
          start_time_.toSecsSinceEpoch(), total_bytes_ });
  if (!ok) return false;
  db_id_ = query.lastInsertId().toLongLong();
  return true;
}

bool Download::Delete() {
  if (!Exists()) return false;
  QSqlQuery query;
  return Sql::ExecParam(&query,
                        "DELETE FROM download WHERE id = ?",
                        { db_id_ });
}

Download::Download(const QSqlRecord& record) {
  db_id_ = record.value("id").toLongLong();
  mime_type_ = record.value("mime_type").toString();
  orig_url_ = record.value("orig_url").toString();
  url_ = record.value("url").toString();
  path_ = record.value("path").toString();
  start_time_ = QDateTime::fromSecsSinceEpoch(
      record.value("start_time").toLongLong(), Qt::UTC);
  auto end_secs = record.value("end_time").toLongLong();
  if (end_secs > 0) {
    end_time_ = QDateTime::fromSecsSinceEpoch(end_secs, Qt::UTC);
  }
  total_bytes_ = record.value("total_bytes").toLongLong();
  // We have to set as complete or canceled when coming from the DB
  current_state_ = record.value("success").toBool() ? Complete : Canceled;
}

void Download::FromCef(CefRefPtr<CefDownloadItem> item) {
  live_id_ = item->GetId();
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
  if (item->GetEndTime().GetTimeT() > 0) {
    end_time_ = QDateTime::fromSecsSinceEpoch(item->GetEndTime().GetTimeT(),
                                              Qt::UTC);
  }
  bytes_received_ = item->GetReceivedBytes();
  total_bytes_ = item->GetTotalBytes();
  current_bytes_per_sec_ = item->GetCurrentSpeed();
}

}  // namespace doogie
