#include "util.h"

QPixmap* Util::CachedPixmap(const QString &resName) {
  auto ret = QPixmapCache::find(resName);
  if (!ret) {
    ret = new QPixmap(resName);
    QPixmapCache::insert(resName, *ret);
  }
  return ret;
}

QIcon Util::CachedIcon(const QString &resName) {
  return QIcon(*CachedPixmap(resName));
}

QIcon Util::CachedIconLighterDisabled(const QString &resName) {
  // I didn't feel some of the disabled icons were lightened enough
  auto pixmap = CachedPixmap(resName);
  auto disabled_key = resName + "_lighter_disabled";
  auto pixmap_disabled = QPixmapCache::find(disabled_key);
  if (!pixmap_disabled) {
    pixmap_disabled = new QPixmap(pixmap->size());
    pixmap_disabled->fill(Qt::transparent);
    QPainter painter;
    painter.begin(pixmap_disabled);
    painter.setOpacity(0.2);
    painter.drawPixmap(0, 0, *pixmap);
    painter.end();
    QPixmapCache::insert(disabled_key, *pixmap_disabled);
  }
  QIcon ret;
  ret.addPixmap(*pixmap);
  ret.addPixmap(*pixmap_disabled, QIcon::Disabled);
  return ret;
}
