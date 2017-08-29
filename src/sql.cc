#include "sql.h"

namespace doogie {

// Easy on/off for debugging
#ifdef QT_DEBUG
const bool kSqlLoggingEnabled = true;
#else
const bool kSqlLoggingEnabled = false;
#endif

const QLoggingCategory Sql::kLoggingCat(
    "sql", kSqlLoggingEnabled ? QtDebugMsg : QtInfoMsg);

bool Sql::EnsureDatabaseSchema() {
  QFile file(":/res/schema.sql");
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qCritical() << "Unable to read schema resource";
    return false;
  }
  auto schema = QString::fromUtf8(file.readAll());
  QSqlQuery query;
  for (auto stmt : schema.split("\n\n")) {
    if (!Exec(&query, stmt)) return false;
  }
  return true;
}

QSqlRecord Sql::ExecSingleParam(QSqlQuery* query,
                                const QString& sql,
                                QVariantList params) {
  if (!ExecParam(query, sql, params)) return QSqlRecord();
  if (!query->next()) {
    DebugLog() << "Single value not found";
    return QSqlRecord();
  }
  return query->record();
}

QSqlRecord Sql::ExecSingleNamedParam(QSqlQuery* query,
                                     const QString& sql,
                                     QVariantHash params) {
  if (!ExecNamedParam(query, sql, params)) return QSqlRecord();
  if (!query->next()) {
    DebugLog() << "Single value not found";
    return QSqlRecord();
  }
  return query->record();
}

bool Sql::ExecParam(QSqlQuery* query,
                    const QString& sql,
                    QVariantList params) {
  DebugLog() << "Executing " << sql << " with params: " << params;
  if (!Prepare(query, sql)) return false;
  for (const auto& param : params) {
    query->addBindValue(param);
  }
  return Exec(query);
}

bool Sql::ExecNamedParam(QSqlQuery* query,
                         const QString& sql,
                         QVariantHash params) {
  DebugLog() << "Executing " << sql << " with params: " << params;
  if (!Prepare(query, sql)) return false;
  for (const auto& param : params.keys()) {
    query->bindValue(param, params[param]);
  }
  return Exec(query);
}

bool Sql::Prepare(QSqlQuery* query, const QString& sql) {
  if (!query->prepare(sql)) {
    qCritical() << "Failed to prepare query: " << query->lastError().text();
    return false;
  }
  return true;
}

bool Sql::Exec(QSqlQuery* query) {
  if (!query->exec()) {
    qCritical() << "Failed to exec query: " << query->lastError().text();
    return false;
  }
  return true;
}

bool Sql::Exec(QSqlQuery* query, const QString& sql) {
  DebugLog() << "Executing " << sql;
  if (!query->exec(sql)) {
    qCritical() << "Failed to exec query: " << query->lastError().text();
    return false;
  }
  return true;
}

QSqlRecord Sql::ExecSingle(QSqlQuery* query, const QString& sql) {
  if (!Exec(query, sql)) return QSqlRecord();
  if (!query->next()) {
    DebugLog() << "Single value not found";
    return QSqlRecord();
  }
  return query->record();
}

Sql::Sql() { }

}  // namespace doogie
