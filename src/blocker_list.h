#ifndef DOOGIE_BLOCKER_LIST_H_
#define DOOGIE_BLOCKER_LIST_H_

#include <QtSql>
#include <QtWidgets>

#include "blocker_rules.h"
#include "cef/cef.h"

namespace doogie {

// DB model representing a single filter list.
class BlockerList {
 public:
  static QList<BlockerList> Lists(QSet<qlonglong> only_ids = {});

  static BlockerList FromFile(const QString& file, bool* ok = nullptr);

  static std::function<void()> FromUrl(
      const Cef& cef,
      const QString& url,
      std::function<void(BlockerList list, bool ok)> callback);

  BlockerList();
  explicit BlockerList(qlonglong id);

  bool Persist();
  bool Delete();
  bool Reload();

  // Caller is owner of resulting rules and is expected to delete them.
  // To get any of the metadata updates, callers must Reload after callback.
  // Note, callback may be called before this returns.
  std::function<void()> LoadRules(
      const Cef& cef,
      int file_index,
      bool local_file_only,
      std::function<void(QList<BlockerRules::Rule*> rules, bool ok)> callback);

  // Caller is owner of resulting rules and is expected to delete them.
  // To get any of the metadata updates, callers must Reload after callback.
  std::function<void()> Update(
      const Cef& cef,
      int file_index,
      std::function<void(QList<BlockerRules::Rule*> rules, bool ok)> callback);

  qlonglong Id() const { return id_; }
  bool Exists() const { return id_ >= 0; }
  QString Name() const { return name_; }
  void SetName(const QString& name) { name_ = name; }
  QString Homepage() const { return homepage_; }
  void SetHomepage(const QString& homepage) { homepage_ = homepage; }
  // May be null if there is only a local path
  QString Url() const { return url_; }
  void SetUrl(const QString& url) { url_ = url; }
  QString LocalPath() const { return local_path_; }
  void SetLocalPath(const QString& local_path) { local_path_ = local_path; }
  QString UrlOrLocalPath() const { return IsLocalOnly() ? local_path_ : url_; }
  bool IsLocalOnly() const { return url_.isEmpty(); }
  // 0 means no version
  qlonglong Version() const { return version_; }
  void SetVersion(qlonglong version) { version_ = version; }
  QDateTime LastRefreshed() const { return last_refreshed_; }
  void SetLastRefreshed(const QDateTime& last_refreshed) {
    last_refreshed_ = last_refreshed;
  }
  // Can be 0 for forced-update only
  int ExpirationHours() const { return expiration_hours_; }
  void SetExpirationHours(int expiration_hours) {
    expiration_hours_ = expiration_hours;
  }
  // 0 for unknown
  qlonglong LastKnownRuleCount() const { return last_known_rule_count_; }
  void SetLastKnownRuleCount(int last_known_rule_count) {
    last_known_rule_count_ = last_known_rule_count;
  }

  bool NeedsUpdate();

 private:
  static QList<BlockerRules::Rule*> RulesFromFile(const QString& file,
                                                  int file_index);
  // Caller is expected to make this live long enough for the callback.
  static std::function<void()> DownloadRules(
      const Cef& cef,
      const QString& url,
      int file_index,
      const QString& file_cache_to,
      std::function<void(QList<BlockerRules::Rule*> rules)> callback);

  explicit BlockerList(const QSqlRecord& record);
  void ApplySqlRecord(const QSqlRecord& record);

  void UpdateFromMeta(BlockerRules::ListMetadata meta);

  qlonglong id_ = -1;
  QString name_;
  QString homepage_;
  QString url_;
  QString local_path_;
  qlonglong version_ = 0;
  QDateTime last_refreshed_;
  int expiration_hours_ = 0;
  qlonglong last_known_rule_count_ = 0;
};

}  // namespace doogie

#endif  // DOOGIE_BLOCKER_LIST_H_
