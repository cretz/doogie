#ifndef DOOGIE_DEBUG_META_SERVER_H_
#define DOOGIE_DEBUG_META_SERVER_H_

#include <QtWidgets>

#include "main_window.h"

namespace doogie {

// Simple debug server that communicates internal state over web sockets
// for use by external testing utilities.
class DebugMetaServer : public QObject {
  Q_OBJECT\

 public:
  explicit DebugMetaServer(MainWindow* parent = nullptr);

 private:
  QJsonObject SaveScreenshot(QJsonObject input);
};

}  // namespace doogie

#endif  // DOOGIE_DEBUG_META_SERVER_H_
