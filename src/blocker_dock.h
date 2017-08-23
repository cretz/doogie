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

  void ProfileUpdated(const Cef& cef);

 private:
  static const int kListLoadTimeoutSeconds = 2 * 60;

  bool IsAllowedToLoad(
        BrowserWidget* browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request);

  BlockerRules::StaticRule::RequestType TypeFromRequest(
      const QUrl& target_url,
      CefRefPtr<CefRequest> request);

  QTableWidget* table_;
  QHash<int, BlockerList> lists_;
  std::atomic<BlockerRules*> rules_;
  qlonglong next_rule_set_unique_num_ = 0;
};

}  // namespace doogie

#endif  // DOOGIE_BLOCKER_DOCK_H_
