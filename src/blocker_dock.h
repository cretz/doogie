#ifndef DOOGIE_BLOCKER_DOCK_H_
#define DOOGIE_BLOCKER_DOCK_H_

#include <QtWidgets>
#include <atomic>

#include "blocker_list.h"
#include "blocker_rules.h"
#include "browser_stack.h"

namespace doogie {

// Dock window for the request blocker.
class BlockerDock : public QDockWidget {
  Q_OBJECT

 public:
  explicit BlockerDock(const Cef& cef,
                       BrowserStack* browser_stack,
                       QWidget* parent = nullptr);
  ~BlockerDock();

  void ProfileUpdated(bool load_local_file_only = false);

 protected:
  void timerEvent(QTimerEvent* event) override;

 private:
  struct BlockedRequest {
    BrowserWidget* browser;
    QUrl target_url;
    QUrl ref_url;
    QString rule_list;
    qlonglong line_number = -1;
    QString rule;
    QDateTime time;
  };

  static const int kListLoadTimeoutSeconds = 2 * 60;
  static const int kCheckUpdatesSeconds = 30 * 60;
  static const int kMaxTableCount = 1000;

  void CheckUpdate();
  bool IsAllowedToLoad(
        BrowserWidget* browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request);

  void RebuildTable();

  // Expects sorting to be off
  void AppendTableRow(const BlockedRequest& request);

  BlockerRules::StaticRule::RequestType TypeFromRequest(
      const QUrl& target_url,
      CefRefPtr<CefRequest> request);

  void SubscribeRuleList(const QString& url);

  const Cef& cef_;
  QTableWidget* table_;
  QCheckBox* current_only_;
  QHash<int, BlockerList> lists_;
  QMutex rules_mutex_;
  BlockerRules* rules_ = nullptr;
  qlonglong next_rule_set_unique_num_ = 0;

  BrowserWidget* current_browser_;
  QHash<BrowserWidget*, QVector<BlockedRequest>> blocked_requests_by_browser_;
  // We'd rather keep this than traverse the above
  QVector<BlockedRequest> blocked_requests_;
};

}  // namespace doogie

#endif  // DOOGIE_BLOCKER_DOCK_H_
