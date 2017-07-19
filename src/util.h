#ifndef DOOGIE_UTIL_H_
#define DOOGIE_UTIL_H_

#include <QtWidgets>

namespace doogie {

class Util {
 public:
  static QPixmap* CachedPixmap(const QString& resName);
  static QIcon CachedIcon(const QString& resName);
  static QIcon CachedIconLighterDisabled(const QString& resName);

  static QJsonObject DebugWidgetGeom(QWidget* widg);
  static QJsonObject DebugWidgetGeom(QWidget* widg, const QRect& rect);
};

}  // namespace doogie

#endif  // DOOGIE_UTIL_H_
