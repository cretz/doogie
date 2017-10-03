#include "blocker_list.h"

#include "profile.h"
#include "sql.h"

namespace doogie {

QList<BlockerList> BlockerList::Lists(QSet<qlonglong> only_ids) {
  QList<BlockerList> ret;
  QSqlQuery query;
  QString sql = "SELECT * FROM blocker_list";
  if (!only_ids.isEmpty()) {
    QString id_list;
    for (auto id : only_ids) {
      if (!id_list.isEmpty()) id_list += ",";
      id_list += QString::number(id);
    }
    sql += QString(" WHERE id IN (%1)").arg(id_list);
  }
  if (!Sql::Exec(&query, sql)) return ret;
  while (query.next()) ret.append(BlockerList(query.record()));
  return ret;
}

BlockerList BlockerList::FromFile(const QString& file, bool* ok) {
  BlockerList list;
  list.local_path_ = file;
  auto rules = RulesFromFile(file, -1);
  if (rules.isEmpty()) {
    if (ok) *ok = false;
    return list;
  }
  auto meta = BlockerRules::GetMetadata(rules);
  qDeleteAll(rules);
  if (meta.rule_count == 0) {
    if (ok) *ok = false;
    return list;
  }
  list.UpdateFromMeta(meta);
  // Remove the URL no matter what the metadata says
  list.url_.clear();
  if (ok) *ok = true;
  return list;
}

std::function<void()> BlockerList::FromUrl(
    const Cef& cef,
    const QString& url,
    std::function<void(BlockerList, bool)> callback) {
  return DownloadRules(cef, url, -1, "", [=](QList<BlockerRules::Rule*> rules) {
    BlockerList list;
    if (rules.isEmpty()) {
      callback(list, false);
      return;
    }
    // Obtain the metadata from the list to build the blocker list
    auto meta = BlockerRules::GetMetadata(rules);
    qDeleteAll(rules);
    if (meta.rule_count == 0) {
      callback(list, false);
      return;
    }
    list.UpdateFromMeta(meta);
    if (list.url_.isEmpty()) list.url_ = url;
    callback(list, true);
  });
}

BlockerList::BlockerList() { }

BlockerList::BlockerList(qlonglong id) : id_(id) {
  if (!Reload()) id_ = -1;
}

bool BlockerList::Persist() {
  // Make a local cache path if not there
  if (local_path_.isEmpty()) {
    // Must have a URL
    if (url_.isEmpty()) {
      qCritical() << "Unable to persist list w/out url or local path";
      return false;
    }
    // Yeah, we know this isn't thread safe, we'll try 5 times
    for (int i = 0; i < 5; i++) {
      auto name = QString("blocker_list_%1.cache").arg(qrand());
      QFile file(QDir(Profile::Current().Path()).filePath(name));
      bool was_created = !file.exists() && file.open(QIODevice::WriteOnly);
      file.close();
      if (was_created) {
        local_path_ = name;
        break;
      }
    }
    if (local_path_.isEmpty()) {
      return false;
    }
  }
  QSqlQuery query;
  auto version = version_ == 0 ? QVariant(QVariant::LongLong) : version_;
  auto expiration_hours = expiration_hours_ == 0 ?
        QVariant(QVariant::Int) : expiration_hours_;
  auto last_known_rule_count = last_known_rule_count_ == 0 ?
        QVariant(QVariant::LongLong) : last_known_rule_count_;
  auto last_refreshed = last_refreshed_.isNull() ?
        QVariant(QVariant::LongLong) : last_refreshed_.toSecsSinceEpoch();
  if (Exists()) {
    return Sql::ExecParam(
        &query,
        "UPDATE blocker_list SET "
        "  name = ?, "
        "  homepage = ?, "
        "  url = ?, "
        "  local_path = ?, "
        "  version = ?, "
        "  last_refreshed = ?, "
        "  expiration_hours = ?,"
        "  last_known_rule_count = ? "
        "WHERE id = ?",
        { name_, homepage_, url_, local_path_, version,
          last_refreshed, expiration_hours, last_known_rule_count, id_ });
  }
  auto ok = Sql::ExecParam(
      &query,
      "INSERT INTO blocker_list ( "
      "  name, homepage, url, local_path, version, "
      "  last_refreshed, expiration_hours, last_known_rule_count "
      ") VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
      { name_, homepage_, url_, local_path_, version,
        last_refreshed, expiration_hours, last_known_rule_count });
  if (!ok) return false;
  id_ = query.lastInsertId().toLongLong();
  return true;
}

bool BlockerList::Delete() {
  if (!Exists()) return false;
  QSqlQuery query;
  auto ok = Sql::ExecParam(&query,
                           "DELETE FROM blocker_list WHERE id = ?",
                           { id_ });
  if (!ok) return false;
  // Try to delete the local cache file if it had a url
  if (!url_.isEmpty()) {
    QFile(local_path_).remove();
  }
  return true;
}

bool BlockerList::Reload() {
  if (!Exists()) return false;
  QSqlQuery query;
  auto record = Sql::ExecSingleParam(
        &query,
        "SELECT * FROM blocker_list WHERE id = ?",
        { id_ });
  if (record.isEmpty()) return false;
  ApplySqlRecord(record);
  return true;
}

std::function<void()> BlockerList::LoadRules(
    const Cef& cef,
    int file_index,
    bool local_file_only,
    std::function<void(QList<BlockerRules::Rule*> rules, bool ok)> callback) {
  std::function<void()> cancel = []() { };
  if (local_file_only || !NeedsUpdate() || url_.isEmpty()) {
    qDebug() << "Attempting to load rules from file" << local_path_ << "first";
    auto rules = RulesFromFile(local_path_, file_index);
    if (!rules.isEmpty()) {
      // If this doesn't have a URL, we consider this a persistable update
      if (url_.isEmpty()) {
        UpdateFromMeta(BlockerRules::GetMetadata(rules));
        // Clear the rule out just in case
        url_.clear();
        last_refreshed_ = QDateTime::currentDateTimeUtc();
        Persist();
      }
      qDebug() << "Rules successfully loaded from file" << local_path_;
      callback(rules, true);
      return cancel;
    }
    if (local_file_only) {
      callback(QList<BlockerRules::Rule*>(), false);
      return cancel;
    }
  }
  qDebug() << "Now trying to load rules from" << url_;
  return Update(cef, file_index, callback);
}

std::function<void()> BlockerList::Update(
    const Cef& cef,
    int file_index,
    std::function<void(QList<BlockerRules::Rule*> rules, bool ok)> callback) {
  // No url, no update
  if (url_.isEmpty()) {
    callback(QList<BlockerRules::Rule*>(), false);
    return []() { };
  }
  auto id = id_;
  // Do NOT capture "this". We don't want to require its presence
  return DownloadRules(
        cef, url_, file_index, local_path_,
        [id, callback](QList<BlockerRules::Rule*> rules) {
    if (rules.isEmpty()) {
      callback(rules, false);
      return;
    }
    // Load up the metadata and set known rule counts and what not
    BlockerList list(id);
    if (list.Exists()) {
      auto meta = BlockerRules::GetMetadata(rules);
      list.UpdateFromMeta(meta);
      list.SetLastRefreshed(QDateTime::currentDateTimeUtc());
      list.Persist();
    }
    callback(rules, true);
  });
}

bool BlockerList::NeedsUpdate() {
  // An empty last-refreshed means we do need an update
  if (last_refreshed_.isNull()) return true;
  return expiration_hours_ > 0 && QDateTime::currentSecsSinceEpoch() >=
      (last_refreshed_.toSecsSinceEpoch() + (expiration_hours_ * 3600));
}

QList<BlockerRules::Rule*> BlockerList::RulesFromFile(const QString& file,
                                                      int file_index) {
  QFile f(file);
  if (!f.open(QIODevice::ReadOnly)) return QList<BlockerRules::Rule*>();
  QTextStream stream(&f);
  bool ok = false;
  auto rules = BlockerRules::ParseRules(&stream, file_index, &ok);
  if (!ok) return QList<BlockerRules::Rule*>();
  return rules;
}

std::function<void()> BlockerList::DownloadRules(
    const Cef& cef,
    const QString& url,
    int file_index,
    const QString& file_cache_to,
    std::function<void(QList<BlockerRules::Rule*>)> callback) {
  auto buf = new QBuffer;
  buf->open(QIODevice::ReadWrite);
  return cef.Download(url, buf,
                       [=](CefRefPtr<CefURLRequest> req, QIODevice* device) {
    if (req->GetRequestStatus() != UR_SUCCESS) {
      qWarning() << "Load of list at" << url <<
                    "failed because non-success of download";
      callback(QList<BlockerRules::Rule*>());
      return;
    }
    // We'd do a lot better streaming this, but meh, it's not a big deal
    auto buf = qobject_cast<QBuffer*>(device);
    if (!buf) {
      qWarning() << "Load of list at" << url <<
                    "failed because device not present";
      callback(QList<BlockerRules::Rule*>());
      return;
    }
    // Put buffer back at beginning for parse
    buf->seek(0);
    // It's mine to delete
    buf->deleteLater();
    // Let's write the entire buffer to the cached file if there is one
    if (!file_cache_to.isEmpty()) {
      QFile file(file_cache_to);
      if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Load of list at" << url <<
                      "failed because unable to write to" << file_cache_to;
        callback(QList<BlockerRules::Rule*>());
        return;
      } else {
        file.write(buf->data());
        file.close();
      }
    }
    auto ok = false;
    QTextStream stream(buf);
    auto rules = BlockerRules::ParseRules(&stream, file_index, &ok);
    if (!ok) {
      qWarning() << "Load of list at" << url <<
                    "failed because of parse failure";
      callback(QList<BlockerRules::Rule*>());
      return;
    }
    qDebug() << "Load of list at" << url <<
                "obtained rule count:" << rules.size();
    callback(rules);
  });
}

BlockerList::BlockerList(const QSqlRecord& record) {
  ApplySqlRecord(record);
}

void BlockerList::ApplySqlRecord(const QSqlRecord& record) {
  id_ = record.value("id").toLongLong();
  name_ = record.value("name").toString();
  homepage_ = record.value("homepage").toString();
  if (homepage_.isEmpty()) homepage_.clear();
  url_ = record.value("url").toString();
  if (url_.isEmpty()) url_.clear();
  local_path_ = record.value("local_path").toString();
  if (!QDir::isAbsolutePath(local_path_)) {
    local_path_ = QDir(Profile::Current().Path()).filePath(local_path_);
  }
  version_ = record.value("version").toLongLong();
  if (!record.isNull("last_refreshed")) {
    last_refreshed_ = QDateTime::fromSecsSinceEpoch(
        record.value("last_refreshed").toLongLong(), Qt::UTC);
  }
  expiration_hours_ = record.value("expiration_hours").toInt();
  last_known_rule_count_ = record.value("last_known_rule_count").toLongLong();
}

void BlockerList::UpdateFromMeta(BlockerRules::ListMetadata meta) {
  name_ = meta.title.isEmpty() ? "" : meta.title;
  if (!meta.homepage.isEmpty()) homepage_ = meta.homepage;
  // We have decided not to support redirect
  // if (!meta.redirect.isEmpty()) url_ = meta.redirect;
  version_ = meta.version;
  expiration_hours_ = meta.expiration_hours;
  last_known_rule_count_ = meta.rule_count;
}

}  // namespace doogie
