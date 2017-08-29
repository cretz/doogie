#include "workspace.h"

#include "sql.h"

namespace doogie {

bool Workspace::WorkspacePage::BubbleInUse(qlonglong bubble_id) {
  QSqlQuery query;
  QString sql = "SELECT EXISTS( "
                " SELECT 1 FROM workspace_page WHERE bubble_id = ? LIMIT 1 "
                ")";
  auto record = Sql::ExecSingleParam(&query, sql, { bubble_id });
  return record.value(0).toInt() == 1;
}

bool Workspace::WorkspacePage::BubbleDeleted(qlonglong bubble_id) {
  QSqlQuery query;
  return Sql::ExecParam(
        &query,
        "UPDATE workspace_page SET bubble_id = -1 WHERE bubble_id = ?",
        { bubble_id });
}

Workspace::WorkspacePage::WorkspacePage(qlonglong id) {
  if (id < 0) return;
  QSqlQuery query;
  FromRecord(Sql::ExecSingleParam(
               &query,
               "SELECT * FROM workspace_page WHERE id = ?",
               { id }));
}

Workspace::WorkspacePage::WorkspacePage(const QSqlRecord& record) {
  FromRecord(record);
}

QIcon Workspace::WorkspacePage::Icon() {
  if (icon_.isNull() && !serialized_icon_.isNull()) {
    // TODO(cretz): handle failure w/ transparent 16x16
    QPixmap pixmap;
    pixmap.loadFromData(serialized_icon_, "PNG");
    icon_ = QIcon(pixmap);
  }
  return icon_;
}

void Workspace::WorkspacePage::SetIcon(const QIcon& icon) {
  icon_ = icon;
  // TODO(cretz): Find a better way to determine the icon is dirty,
  //  maybe store the URL? But some favicons change
  serialized_icon_.clear();
}

bool Workspace::WorkspacePage::Persist() {
  // If serialized icon is not there but an icon is, we create it
  if (serialized_icon_.isNull() && !icon_.isNull()) {
    QBuffer buffer(&serialized_icon_);
    buffer.open(QIODevice::WriteOnly);
    icon_.pixmap(16, 16).save(&buffer, "PNG");
  }
  QSqlQuery query;
  QVariant parent_id = parent_id_ < 0 ?
        QVariant(QVariant::LongLong) : parent_id_;
  if (Exists()) {
    return Sql::ExecParam(
          &query,
          "UPDATE workspace_page SET "
          " workspace_id = ?, "
          " parent_id = ?, "
          " pos = ?, "
          " icon = ?, "
          " title = ?, "
          " url = ?, "
          " bubble_id = ?, "
          " suspended = ?, "
          " expanded = ? "
          "WHERE id = ?",
          { workspace_id_, parent_id, pos_, serialized_icon_,
            title_, url_, bubble_id_, suspended_, expanded_, id_ });
  }
  auto ok = Sql::ExecParam(
      &query,
      "INSERT INTO workspace_page ( "
      "   workspace_id, parent_id, pos, icon, "
      "   title, url, bubble_id, suspended, expanded "
      ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      { workspace_id_, parent_id, pos_, serialized_icon_,
        title_, url_, bubble_id_, suspended_, expanded_ });
  if (!ok) return false;
  id_ = query.lastInsertId().toLongLong();
  return true;
}

bool Workspace::WorkspacePage::Delete() {
  if (id_ < 0) return false;
  QSqlQuery query;
  auto ok = Sql::ExecParam(&query,
                           "DELETE FROM workspace_page WHERE id = ?",
                           { id_ });
  if (ok) id_ = -1;
  return ok;
}

void Workspace::WorkspacePage::FromRecord(const QSqlRecord& record) {
  if (record.isEmpty()) return;
  workspace_id_ = record.value("workspace_id").toLongLong();
  id_ = record.value("id").toLongLong();
  parent_id_ = record.isNull("parent_id") ?
        -1 : record.value("parent_id").toLongLong();
  pos_ = record.value("pos").toInt();
  serialized_icon_ = record.value("icon").toByteArray();
  title_ = record.value("title").toString();
  url_ = record.value("url").toString();
  bubble_id_ = record.value("bubble_id").toLongLong();
  suspended_ = record.value("suspended").toBool();
  expanded_ = record.value("expanded").toBool();
}

QList<Workspace> Workspace::Workspaces() {
  QList<Workspace> ret;
  QSqlQuery query;
  if (!Sql::Exec(&query, "SELECT * FROM workspace ORDER BY name")) return ret;
  while (query.next()) ret.append(Workspace(query.record()));
  return ret;
}

QList<Workspace> Workspace::OpenWorkspaces() {
  QList<Workspace> ret;
  QSqlQuery query;
  if (!Sql::Exec(&query,
                 "SELECT * FROM workspace "
                 "WHERE open_index IS NOT NULL "
                 "ORDER BY open_index")) return ret;
  while (query.next()) ret.append(Workspace(query.record()));
  return ret;
}

QList<Workspace> Workspace::RecentWorkspaces(QSet<qlonglong> excluding,
                                             int count) {
  QString sql = "SELECT * FROM workspace ";
  if (!excluding.isEmpty()) {
    sql += " WHERE id NOT IN (";
    QStringList exclude_ids;
    for (auto exclude : excluding) {
      exclude_ids.append(QString::number(exclude));
    }
    sql += exclude_ids.join(',') + ") ";
  }
  sql += QString(" ORDER BY last_opened DESC LIMIT %1").arg(count);
  QList<Workspace> ret;
  QSqlQuery query;
  if (!Sql::Exec(&query, sql)) return ret;
  while (query.next()) ret.append(Workspace(query.record()));
  return ret;
}

QString Workspace::NextUnusedWorkspaceName() {
  QSqlQuery query;
  Sql::Exec(&query, "SELECT COUNT(1) FROM workspace");
  if (!query.next()) return "Workspace 1";
  int count = query.value(0).toInt();
  QString name;
  do {
    name = QString("Workspace %1").arg(++count);
  } while (NameInUse(name));
  return name;
}

bool Workspace::NameInUse(const QString& name) {
  QSqlQuery query;
  return !Sql::ExecSingleParam(&query,
                               "SELECT 1 FROM workspace WHERE name = ?",
                               { name }).isEmpty();
}

bool Workspace::UpdateOpenWorkspaces(QList<qlonglong> ids) {
  QSqlQuery query;
  if (!Sql::Exec(&query, "UPDATE workspace SET open_index = NULL")) {
    return false;
  }
  for (int i = 0; i < ids.size(); i++) {
    if (!Sql::Exec(&query, QString("UPDATE workspace "
                                   "SET open_index = %1 "
                                   "WHERE id = %2").arg(i).arg(ids[i]))) {
        return false;
    }
  }
  return true;
}

Workspace::Workspace(qlonglong id) {
  if (id < 0) return;
  QSqlQuery query;
  FromRecord(Sql::ExecSingleParam(&query,
                                  "SELECT * FROM workspace WHERE id = ?",
                                  { id }));
}

Workspace::Workspace(const QSqlRecord& record) {
  if (record.isEmpty()) return;
  FromRecord(record);
}

bool Workspace::Persist() {
  QSqlQuery query;
  if (Exists()) {
    return Sql::ExecParam(
          &query,
          "UPDATE workspace SET "
          " name = ?, "
          " last_opened = ? "
          "WHERE id = ?",
          { name_, last_opened_, id_ });
  }
  auto ok = Sql::ExecParam(
        &query,
        "INSERT INTO workspace (name, last_opened) "
        "VALUES (?, ?)",
        { name_, last_opened_ });
  if (!ok) return false;
  id_ = query.lastInsertId().toLongLong();
  return true;
}

bool Workspace::Delete() {
  if (id_ < 0) return false;
  // Delete children first
  QSqlDatabase::database().transaction();
  QSqlQuery query;
  auto ok = Sql::ExecParam(&query,
                           "DELETE FROM workspace_page WHERE workspace_id = ?",
                           { id_ });
  if (ok) {
    ok = Sql::ExecParam(&query,
                       "DELETE FROM workspace WHERE id = ?",
                        { id_ });
  }
  if (!ok) {
    QSqlDatabase::database().rollback();
    return false;
  }
  QSqlDatabase::database().commit();
  id_ = -1;
  return true;
}

QList<Workspace::WorkspacePage> Workspace::AllChildren() const {
  QList<Workspace::WorkspacePage> ret;
  QSqlQuery query;
  auto ok = Sql::ExecParam(
        &query,
        "SELECT * FROM workspace_page "
        "WHERE workspace_id = ? "
        "ORDER BY pos",
        { id_ });
  if (!ok) return ret;
  while (query.next()) ret.append(Workspace::WorkspacePage(query.record()));
  return ret;
}

QList<Workspace::WorkspacePage> Workspace::ChildrenOf(
    qlonglong parent_id) const {
  QList<Workspace::WorkspacePage> ret;
  QSqlQuery query;
  auto ok = Sql::ExecParam(
        &query,
        "SELECT * FROM workspace_page "
        "WHERE workspace_id = ? AND parent_id = ? "
        "ORDER BY pos",
        { id_, parent_id < 0 ? QVariant(QVariant::LongLong) : parent_id });
  if (!ok) return ret;
  while (query.next()) ret.append(Workspace::WorkspacePage(query.record()));
  return ret;
}

void Workspace::FromRecord(const QSqlRecord& record) {
  if (record.isEmpty()) return;
  id_ = record.value("id").toLongLong();
  name_ = record.value("name").toString();
  last_opened_ = record.value("last_opened").toLongLong();
}

}  // namespace doogie
