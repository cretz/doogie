#ifndef DOOGIE_DOWNLOADS_DOCK_H_
#define DOOGIE_DOWNLOADS_DOCK_H_

#include <QtWidgets>

#include "browser_stack.h"

namespace doogie {

// Dock window for the current downloads.
class DownloadsDock : public QDockWidget {
  Q_OBJECT
 public:
  explicit DownloadsDock(BrowserStack* browser_stack,
                         QWidget* parent = nullptr);

  bool HasActiveDownload();

 private:
  QListWidget* list_;
};

}  // namespace doogie

#endif  // DOOGIE_DOWNLOADS_DOCK_H_
