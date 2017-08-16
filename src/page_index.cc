#include "page_index.h"

#include "sql.h"
#include "util.h"

namespace doogie {

PageIndex::Expirer::Expirer() {
  timer.connect(&timer, &QTimer::timeout, PageIndex::DoExpiration);
  // 3 minutes for now
  timer.start(3 * 60 * 1000);
}

QList<PageIndex::AutocompletePage> PageIndex::AutocompleteSuggest(
    const QString& text, int count) {
  // Take the text, trim, split on space, ignore empties, add asterisk
  // after each item, rejoin, search...
  // Oh, and we quote it, which means replace existing quotes w/ double quotes
  auto text_to_split = text;
  // We need to take off the scheme if it's there
  auto scheme_colon_index = text.indexOf("://");
  if (scheme_colon_index >= 0) {
    text_to_split = text.mid(scheme_colon_index + 3);
  }
  QString to_search = "";
  for (auto piece : text_to_split.trimmed().replace("'", "''").
       split(' ', QString::SkipEmptyParts)) {
    to_search += QString("\"%1\"*").arg(piece);
  }
  QList<PageIndex::AutocompletePage> ret;
  if (to_search.length() == 1) return ret;
  QSqlQuery query;
  auto sql = QString(
      "SELECT ap.url, ap.title, f.url as favicon_url "
      "FROM autocomplete_page_fts('%1') apf "
      "  JOIN autocomplete_page ap ON "
      "    ap.id = apf.rowid "
      "  LEFT JOIN favicon f "
      "    ON f.id = ap.favicon_id "
      "ORDER BY apf.frecency DESC "
      "LIMIT %2").arg(to_search).arg(count);
  if (!Sql::Exec(&query, sql)) return ret;
  while (query.next()) {
    ret.append(PageIndex::AutocompletePage {
        query.value("url").toString(),
        query.value("title").toString(),
        CachedFavicon(query.value("favicon_url").toString())
    });
  }
  return ret;
}

bool PageIndex::MarkVisit(const QString& url,
                          const QString& title,
                          const QString& favicon_url,
                          const QIcon& favicon) {
  // We are doing this by URL hash and then URL (for collisions).
  // We don't expect *that* much contention so we'll do an update
  // and if 0 affected, then insert. Since we don't put the URL
  // itself into a unique index, we cannot use insert-or-replace.
  // We accept the obvious race condition that exists here and
  // trust the caller only does this in one thread.
  auto scheme_sep = url.indexOf("://");
  if (scheme_sep == -1) return false;
  auto hash = Util::HashString(url);
  auto schemeless_url = url.mid(scheme_sep + 3);
  auto curr_secs = QDateTime::currentSecsSinceEpoch();
  auto favicon_id = FaviconId(favicon_url, favicon);
  QSqlQuery query;
  auto ok = Sql::ExecParam(
      &query,
      "UPDATE autocomplete_page SET "
      "  schemeless_url = ?, "
      "  title = ?, "
      "  favicon_id = ?, "
      "  last_visited = ?, "
      "  visit_count = visit_count + 1, "
      "  frecency = ? + ((visit_count + 1) * ?) "
      "WHERE url_hash = ? AND url = ? ",
      { schemeless_url, title, favicon_id, curr_secs,
        curr_secs, kVisitTimeWorthSeconds, hash, url });
  if (!ok) return false;
  if (query.numRowsAffected() > 0) return true;
  // Try an insert
  return Sql::ExecParam(
    &query,
    "INSERT INTO autocomplete_page ( "
    "  url, url_hash, schemeless_url, title, "
    "  favicon_id, last_visited, visit_count, frecency "
    ") VALUES (?, ?, ?, ?, ?, ?, ?, ?) ",
    { url, hash, schemeless_url, title, favicon_id,
      curr_secs, 1, curr_secs + kVisitTimeWorthSeconds });
}

bool PageIndex::UpdateTitle(const QString& url, const QString& title) {
  QSqlQuery query;
  return Sql::ExecParam(
      &query,
      "UPDATE autocomplete_page SET title = ? "
      "WHERE url_hash = ? and url = ?",
      { title, Util::HashString(url), url });
}

bool PageIndex::UpdateFavicon(const QString& url,
                              const QString& favicon_url,
                              const QIcon& favicon) {
  QSqlQuery query;
  return Sql::ExecParam(
      &query,
      "UPDATE autocomplete_page SET favicon_id = ? "
      "WHERE url_hash = ? AND url = ?",
      { FaviconId(favicon_url, favicon), Util::HashString(url), url });
}

QIcon PageIndex::CachedFavicon(const QString& url) {
  if (url.isEmpty()) return QIcon();
  // We'll just use the pixmap cache directly here
  QPixmap pixmap;
  auto key = QString("doogie:favicon_") + url;
  if (!QPixmapCache::find(key, &pixmap)) {
    QSqlQuery query;
    auto hash = Util::HashString(url);
    auto rec = Sql::ExecSingleParam(
          &query,
          "SELECT data FROM favicon "
          "WHERE url_hash = ? AND url = ?",
          { hash, url });
    if (rec.isEmpty()) return QIcon();
    pixmap.loadFromData(rec.value(0).toByteArray(), "PNG");
    QPixmapCache::insert(key, pixmap);
  }
  return QIcon(pixmap);
}

void PageIndex::DoExpiration() {
  QSqlQuery query;
  auto old =
      QDateTime::currentSecsSinceEpoch() - kExpireNotVisitedSinceSeconds;
  Sql::ExecParam(
        &query,
        "DELETE FROM autocomplete_page WHERE last_visited < ?",
        { old });
  Sql::Exec(
        &query,
        // Meh, we know it's slow...put it in the delete trigger instead?
        "DELETE FROM favicon WHERE id NOT IN ( "
        "  SELECT DISTINCT favicon_id "
        "  FROM autocomplete_page "
        ")");
}

QVariant PageIndex::FaviconId(const QString& url, const QIcon& favicon) {
  if (url.isEmpty() || favicon.isNull()) return QVariant(QVariant::LongLong);
  auto hash = Util::HashString(url);
  QSqlQuery query;
  auto icon_bytes = [=]() -> QByteArray {
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    favicon.pixmap(16, 16).save(&buffer, "PNG");
    return buffer.data();
  };
  auto record = Sql::ExecSingleParam(
      &query,
      "SELECT id "
      "FROM favicon "
      "WHERE url_hash = ? AND url = ?",
      { hash, url });
  if (query.lastError().isValid()) return QVariant(QVariant::LongLong);
  if (!record.isEmpty()) return record.value("id");
  // Not there, insert
  QBuffer buffer;
  buffer.open(QIODevice::WriteOnly);
  favicon.pixmap(16, 16).save(&buffer, "PNG");
  auto ok = Sql::ExecParam(
      &query,
      "INSERT INTO favicon ( "
      "  url, url_hash, data "
      ") VALUES (?, ?, ?)",
      { url, hash, buffer.data() });
  if (!ok) return QVariant(QVariant::LongLong);
  return query.lastInsertId();
}

}  // namespace doogie
