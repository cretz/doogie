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

QString Util::FriendlyByteSize(double num) {
  // Ref: https://stackoverflow.com/questions/30958043/qt-easiest-way-to-convert-int-to-file-size-format-kb-mb-or-gb
  QStringList list = { "KB", "MB", "GB", "TB" };
  QStringListIterator i(list);
  QString unit = "bytes";
  while (num >= 1024.0 && i.hasNext()) {
    unit = i.next();
    num /= 1024.0;
  }
  return QString::number(num, 'f', 2) + " " + unit;
}

QString Util::FriendlyTimeSpan(qint64 seconds) {
  if (seconds < 60) return QString("%1s").arg(seconds);
  if (seconds < 3600) {
    return QString("%1m %2s").arg(seconds / 60).arg(seconds % 60);
  }
  if (seconds < 24 * 3600) {
    auto after_hour_seconds = seconds % 3600;
    return QString("%1h %2m %3s").arg(seconds / 3600).
        arg(after_hour_seconds / 60).arg(after_hour_seconds % 60);
  }
  auto after_day_seconds = seconds % (24 * 3600);
  auto after_hour_seconds = after_day_seconds % 3600;
  return QString("%1d %2h %3m %4s").arg(seconds / (24 * 3600)).
      arg(after_day_seconds / 3600).
      arg(after_hour_seconds / 60).
      arg(after_hour_seconds % 60);
}

void Util::RunOnMainThread(std::function<void()> fn) {
  // A bit ghetto to rely on the stack destruction but meh
  // Ref: https://stackoverflow.com/questions/21646467/how-to-execute-a-functor-or-a-lambda-in-a-given-thread-in-qt-gcd-style
  QObject obj;
  QObject::connect(&obj, &QObject::destroyed, QApplication::instance(),
                   fn, Qt::QueuedConnection);
}

}  // namespace doogie
