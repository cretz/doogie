#ifndef DOOGIE_UTIL_H_
#define DOOGIE_UTIL_H_

#include <QtWidgets>

class Util
{
public:
  static QPixmap* CachedPixmap(const QString &resName);

  static QIcon CachedIcon(const QString &resName);

  static QIcon CachedIconLighterDisabled(const QString &resName);
};

#endif // DOOGIE_UTIL_H_
