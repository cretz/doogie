#include "util.h"

namespace doogie {

bool Util::OpenContainingFolder(const QString& path) {
  auto explorer = QStandardPaths::findExecutable("explorer.exe");
  if (explorer.isEmpty()) return false;
  auto param = QDir::toNativeSeparators(path);
  if (!QFileInfo(path).isDir()) param = QString("/select,") + param;
  return QProcess::startDetached(explorer + " " + param);
}

QString Util::ExePath() {
  wchar_t buffer[MAX_PATH];
  auto ret = GetModuleFileName(nullptr, buffer, MAX_PATH);
  if (ret == 0) return QString();
  return QString::fromWCharArray(buffer, ret);
}

}  // namespace doogie
