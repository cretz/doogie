#include "util.h"

namespace doogie {

bool Util::OpenContainingFolder(const QString& path) {
  QFileInfo info(path);
  return QDesktopServices::openUrl(
    QUrl::fromLocalFile(info.isDir() ? path : info.path()));
}

}  // namespace doogie
