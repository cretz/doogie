#ifndef DOOGIE_UTIL_H_
#define DOOGIE_UTIL_H_

#include <QtWidgets>

namespace doogie {

class Util {
 public:
  enum SettingState {
    Default,
    Enabled,
    Disabled
  };

  static QPixmap* CachedPixmap(const QString& res_name);
  static QIcon CachedIcon(const QString& res_name);
  static QIcon CachedIconLighterDisabled(const QString& res_name);
  static void LighterDisabled(const QPixmap& source, QPixmap* dest);
  static QPixmap* CachedPixmapColorOverlay(const QString& res_name,
                                           const QColor& color);

  static QJsonObject DebugWidgetGeom(const QWidget* widg);
  static QJsonObject DebugWidgetGeom(const QWidget* widg, const QRect& rect);
  static QJsonObject DebugRect(const QPoint& top_left, const QSize& size);

  static QKeySequence KeySequenceOrEmpty(const QString& str);
};

}  // namespace doogie

#endif  // DOOGIE_UTIL_H_
