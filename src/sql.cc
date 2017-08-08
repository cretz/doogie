#include "sql.h"

namespace doogie {

const QLoggingCategory Sql::kLoggingCat(
    "sql", Sql::kLoggingEnabled ? QtDebugMsg : QtInfoMsg);

QSqlRecord Sql::ExecSingleParam(QSqlQuery& query,
                                const QString& sql,
                                QVariantList params) {
  if (!ExecParam(query, sql, params)) return QSqlRecord();
  if (!query.next()) {
    DebugLog() << "Single value not found";
    return QSqlRecord();
  }
  return query.record();
}

QSqlRecord Sql::ExecSingleNamedParam(QSqlQuery& query,
                                     const QString& sql,
                                     QVariantHash params) {
  if (!ExecNamedParam(query, sql, params)) return QSqlRecord();
  if (!query.next()) {
    DebugLog() << "Single value not found";
    return QSqlRecord();
  }
  return query.record();
}

bool Sql::ExecParam(QSqlQuery& query,
                    const QString& sql,
                    QVariantList params) {
  DebugLog() << "Executing " << sql << " with params: " << params;
  if (!Prepare(query, sql)) return false;
  for (const auto& param : params) query.addBindValue(param);
  return Exec(query);
}

bool Sql::ExecNamedParam(QSqlQuery& query,
                         const QString& sql,
                         QVariantHash params) {
  DebugLog() << "Executing " << sql << " with params: " << params;
  if (!Prepare(query, sql)) return false;
  for (const auto& param : params.keys()) {
    query.bindValue(param, params[param]);
  }
  return Exec(query);
}

bool Sql::Prepare(QSqlQuery& query, const QString& sql) {
  if (!query.prepare(sql)) {
    qCritical() << "Failed to prepare query: " << query.lastError().text();
    return false;
  }
  return true;
}

bool Sql::Exec(QSqlQuery& query) {
  if (!query.exec()) {
    qCritical() << "Failed to exec query: " << query.lastError().text();
    return false;
  }
  return true;
}

bool Sql::Exec(QSqlQuery& query, const QString& sql) {
  DebugLog() << "Executing " << sql;
  if (!query.exec(sql)) {
    qCritical() << "Failed to exec query: " << query.lastError().text();
    return false;
  }
  return true;
}

Sql::Sql() { }

}  // namespace doogie
