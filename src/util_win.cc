#include "util.h"

namespace doogie {

bool Util::OpenContainingFolder(const QString& path) {
  auto explorer = QStandardPaths::findExecutable("explorer.exe");
  if (explorer.isEmpty()) return false;
  auto param = QDir::toNativeSeparators(path);
  if (!QFileInfo(path).isDir()) param = QString("/select,") + param;
  return QProcess::startDetached(explorer + " " + param);
}

}  // namespace doogie
