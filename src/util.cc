#include "util.h"

namespace doogie {

QPixmap* Util::CachedPixmap(const QString& resName) {
  auto ret = QPixmapCache::find(resName);
  if (!ret) {
    ret = new QPixmap(resName);
    QPixmapCache::insert(resName, *ret);
  }
  return ret;
}

QIcon Util::CachedIcon(const QString& resName) {
  return QIcon(*CachedPixmap(resName));
}

QIcon Util::CachedIconLighterDisabled(const QString& resName) {
  // I didn't feel some of the disabled icons were lightened enough
  auto pixmap = CachedPixmap(resName);
  auto disabled_key = resName + "_lighter_disabled";
  auto pixmap_disabled = QPixmapCache::find(disabled_key);
  if (!pixmap_disabled) {
    pixmap_disabled = new QPixmap(pixmap->size());
    Util::LighterDisabled(*pixmap, pixmap_disabled);
    QPixmapCache::insert(disabled_key, *pixmap_disabled);
  }
  QIcon ret;
  ret.addPixmap(*pixmap);
  ret.addPixmap(*pixmap_disabled, QIcon::Disabled);
  return ret;
}

void Util::LighterDisabled(const QPixmap& source, QPixmap* dest) {
  dest->fill(Qt::transparent);
  QPainter painter;
  painter.begin(dest);
  painter.setOpacity(0.2);
  painter.drawPixmap(0, 0, source);
  painter.end();
}

QJsonObject Util::DebugWidgetGeom(QWidget* widg) {
  return Util::DebugWidgetGeom(widg, widg->rect());
}

QJsonObject Util::DebugWidgetGeom(QWidget* widg, const QRect& rect) {
  return Util::DebugRect(widg->mapToGlobal(rect.topLeft()), rect.size());
}

QJsonObject Util::DebugRect(const QPoint& topLeft, const QSize& size) {
  return {
    {"x", topLeft.x() },
    {"y", topLeft.y() },
    {"w", size.width() },
    {"h", size.height() }
  };
}

}  // namespace doogie
