#ifndef DOOGIE_BLOCKER_DOCK_H_
#define DOOGIE_BLOCKER_DOCK_H_

#include <atomic>
#include <QtWidgets>

#include "blocker_list.h"
#include "blocker_rules.h"
#include "browser_stack.h"

namespace doogie {

class BlockerDock : public QDockWidget {
  Q_OBJECT
 public:
  explicit BlockerDock(const Cef& cef,
                       BrowserStack* browser_stack,
                       QWidget* parent = nullptr);

  void ProfileUpdated(bool load_local_file_only = false);

 protected:
  void timerEvent(QTimerEvent* event) override;

 private:
  static const int kListLoadTimeoutSeconds = 2 * 60;
  static const int kCheckUpdatesSeconds = 30 * 60;

  void CheckUpdate();
  bool IsAllowedToLoad(
        BrowserWidget* browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request);

  BlockerRules::StaticRule::RequestType TypeFromRequest(
      const QUrl& target_url,
      CefRefPtr<CefRequest> request);

  const Cef& cef_;
  QTableWidget* table_;
  QHash<int, BlockerList> lists_;
  std::atomic<BlockerRules*> rules_;
  qlonglong next_rule_set_unique_num_ = 0;
};

}  // namespace doogie

#endif  // DOOGIE_BLOCKER_DOCK_H_
