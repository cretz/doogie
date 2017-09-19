#include "util.h"

#include <unistd.h>

namespace doogie {

bool Util::OpenContainingFolder(const QString& path) {
  QFileInfo info(path);
  return QDesktopServices::openUrl(
    QUrl::fromLocalFile(info.isDir() ? path : info.path()));
}

QString Util::ExePath() {
  QFileInfo pfi(QString::fromLatin1("/proc/%1/exe").arg(::getpid()));
  if (!pfi.exists() || !pfi.isSymLink()) return QString();
  return pfi.canonicalFilePath();
}

}  // namespace doogie
