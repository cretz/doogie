#ifndef DOOGIE_SQL_H_
#define DOOGIE_SQL_H_

#include <QtSql>
#include <QtWidgets>

namespace doogie {

class Sql {
 public:
  static QSqlRecord ExecSingleParam(QSqlQuery& query,
                                    const QString& sql,
                                    QVariantList params);
  static QSqlRecord ExecSingleNamedParam(QSqlQuery& query,
                                         const QString& sql,
                                         QVariantHash params);

  static bool ExecParam(QSqlQuery& query,
                        const QString& sql,
                        QVariantList params);
  static bool ExecNamedParam(QSqlQuery& query,
                             const QString& sql,
                             QVariantHash params);

  static bool Prepare(QSqlQuery& query, const QString& sql);
  static bool Exec(QSqlQuery& query);
  static bool Exec(QSqlQuery& query, const QString& sql);

 private:
  // Easy on/off for debugging
  static const bool kLoggingEnabled = false;
  static const QLoggingCategory kLoggingCat;

  static QDebug DebugLog() { return qDebug(kLoggingCat); }

  Sql();
};

}

#endif // DOOGIE_SQL_H_
