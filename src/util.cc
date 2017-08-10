#include "util.h"

namespace doogie {

QPixmap* Util::CachedPixmap(const QString& res_name) {
  auto ret = QPixmapCache::find(res_name);
  if (!ret) {
    ret = new QPixmap(res_name);
    QPixmapCache::insert(res_name, *ret);
  }
  return ret;
}

QIcon Util::CachedIcon(const QString& res_name) {
  return QIcon(*CachedPixmap(res_name));
}

QIcon Util::CachedIconLighterDisabled(const QString& res_name) {
  // I didn't feel some of the disabled icons were lightened enough
  auto pixmap = CachedPixmap(res_name);
  auto disabled_key = res_name + "_lighter_disabled";
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

QPixmap* Util::CachedPixmapColorOverlay(const QString& res_name,
                                        const QColor& color) {
  auto color_key = res_name + "__" + color.name();
  auto pixmap = QPixmapCache::find(color_key);
  if (!pixmap) {
    auto orig = CachedPixmap(res_name);
    pixmap = new QPixmap(orig->size());
    pixmap->fill(Qt::transparent);
    QPainter painter(pixmap);
    painter.drawPixmap(0, 0, *orig);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap->rect(), color);
    painter.end();
    QPixmapCache::insert(color_key, *pixmap);
  }
  return pixmap;
}

QJsonObject Util::DebugWidgetGeom(const QWidget* widg) {
  return Util::DebugWidgetGeom(widg, widg->rect());
}

QJsonObject Util::DebugWidgetGeom(const QWidget* widg, const QRect& rect) {
  return Util::DebugRect(widg->mapToGlobal(rect.topLeft()), rect.size());
}

QJsonObject Util::DebugRect(const QPoint& top_left, const QSize& size) {
  return {
    {"x", top_left.x() },
    {"y", top_left.y() },
    {"w", size.width() },
    {"h", size.height() }
  };
}

QKeySequence Util::KeySequenceOrEmpty(const QString& str) {
  if (str.isEmpty()) return QKeySequence();
  auto seq = QKeySequence::fromString(str);
  if (seq.isEmpty()) return QKeySequence();
  for (int i = 0; i < seq.count(); i++) {
    if (seq[i] == Qt::Key_unknown) return QKeySequence();
  }
  return seq;
}

quint64 Util::HashString(const QString& str) {
  // FNV-1a 64 bit
  quint64 hash = 14695981039346656037ull;
  for (auto byte : str.toLatin1()) {
    hash = (hash ^ byte) * 109951162821ull;
  }
  return hash;
}

}  // namespace doogie
