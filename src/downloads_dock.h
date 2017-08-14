#ifndef DOOGIE_DOWNLOADS_DOCK_H_
#define DOOGIE_DOWNLOADS_DOCK_H_

#include <QtWidgets>

#include "browser_stack.h"

namespace doogie {

class DownloadsDock : public QDockWidget {
  Q_OBJECT
 public:
  explicit DownloadsDock(BrowserStack* browser_stack,
                         QWidget* parent = nullptr);
};

}  // namespace doogie

#endif  // DOOGIE_DOWNLOADS_DOCK_H_
