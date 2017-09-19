#ifndef DOOGIE_UTIL_H_
#define DOOGIE_UTIL_H_

#include <QtWidgets>

namespace doogie {

// Simple utilities for Qt.
class Util {
 public:
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

  static quint64 HashString(const QString& str);

  static QString FriendlyByteSize(double num);
  static QString FriendlyTimeSpan(qint64 seconds);

  static void RunOnMainThread(std::function<void()> fn);

  static bool OpenContainingFolder(const QString& path);

  // Unlike QCoreApplication::applicationFilePath, this does not require a Qt
  //  application instance to obtain the path. Null string on error.
  static QString ExePath();
};

}  // namespace doogie

#endif  // DOOGIE_UTIL_H_
