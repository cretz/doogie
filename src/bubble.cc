#include "bubble.h"

#include "profile.h"
#include "sql.h"
#include "util.h"

namespace doogie {

Bubble Bubble::DefaultBubble() {
  return CachedBubbles().first();
}

QList<Bubble> Bubble::CachedBubbles() {
  if (cached_bubbles_.isEmpty()) {
    QSqlQuery query;
    Sql::Exec(&query, "SELECT * FROM bubble ORDER BY order_index");
    while (query.next()) cached_bubbles_.append(Bubble(query.record()));
    // If the list is still empty, we implicitly insert a persisted default
    if (cached_bubbles_.isEmpty()) {
      Bubble bubble;
      if (!bubble.Persist()) {
        qCritical() << "Unable to create default bubble";
        return { };
      }
      cached_bubbles_.append(bubble);
    }
  }
  return cached_bubbles_;
}

Bubble Bubble::FromId(qlonglong id, bool* ok) {
  for (const auto& bubble : CachedBubbles()) {
    if (bubble.Id() == id) {
      if (ok) *ok = true;
      return bubble;
    }
  }
  if (ok) *ok = false;\
  return Bubble();
}

void Bubble::InvalidateCachedBubbles() {
  cached_bubbles_.clear();
}

bool Bubble::ResetOrderIndexes() {
  QSqlQuery query;
  return Sql::Exec(&query, "UPDATE bubble SET order_index = -id");
}

Bubble::Bubble() {
  name_ = "";
}

bool Bubble::Persist() {
  auto ok = false;
  QSqlQuery query;
  if (Exists()) {
    ok = Sql::ExecParam(
        &query,
        "UPDATE bubble SET "
        "  order_index = ?, "
        "  name = ?, "
        "  cache_path = ?, "
        "  icon_path = ?, "
        "  icon_color = ?,"
        "  overrides_blocker_lists = ? "
        "WHERE id = ?",
        { order_index_, name_, cache_path_, icon_path_,
          icon_color_, overrides_blocker_lists_, id_ });
    if (!ok) return false;
  } else {
    ok = Sql::ExecParam(
        &query,
        "INSERT INTO bubble ( "
        "  order_index, name, cache_path, icon_path, "
        "  icon_color, overrides_blocker_lists "
        ") VALUES (?, ?, ?, ?, ?, ?)",
        { order_index_, name_, cache_path_, icon_path_,
          icon_color_, overrides_blocker_lists_ });
    if (!ok) return false;
    id_ = query.lastInsertId().toLongLong();
  }
  return PersistBrowserSettings(browser_settings_, id_) &&
      PersistEnabledBlockerListIds(enabled_blocker_list_ids_, id_);
}

bool Bubble::Delete() {
  if (!Exists()) return false;
  QSqlQuery query;
  return Sql::ExecParam(&query,
                        "DELETE FROM bubble WHERE id = ?",
                        { id_ });
}

bool Bubble::Reload() {
  if (!Exists()) return false;
  QSqlQuery query;
  auto record = Sql::ExecSingleParam(
        &query,
        "SELECT * FROM bubble WHERE id = ?",
        { id_ });
  if (record.isEmpty()) return false;
  ApplySqlRecord(record);
  return true;
}

void Bubble::CopySettingsFrom(const Bubble& other) {
  // We do not copy the ID, but we do copy the already-generated icon
  name_ = other.name_;
  icon_ = other.icon_;
  icon_path_ = other.icon_path_;
  icon_color_ = other.icon_color_;
  cache_path_ = other.cache_path_;
  browser_settings_ = other.browser_settings_;
  overrides_blocker_lists_ = other.overrides_blocker_lists_;
  enabled_blocker_list_ids_ = other.enabled_blocker_list_ids_;
}

void Bubble::ApplyCefBrowserSettings(CefBrowserSettings* settings,
                                     bool include_current_profile) const {
  if (include_current_profile) {
    Profile::Current().ApplyCefBrowserSettings(settings);
  }

  ApplyBrowserSettings(browser_settings_, settings);
}

void Bubble::ApplyCefRequestContextSettings(
    CefRequestContextSettings* settings,
    bool include_current_profile) const {
  if (include_current_profile) {
    Profile::Current().ApplyCefRequestContextSettings(settings);
  }
  ApplyBrowserSettings(browser_settings_, settings);

  if (!cache_path_.isNull()) {
    // Empty/null is disabled, otherwise qualify w/ profile dir if necessary
    if (!cache_path_.isEmpty()) {
      CefString(&settings->cache_path) =
          QDir::toNativeSeparators(QDir::cleanPath(QDir(
              Profile::Current().Path()).filePath(cache_path_))).toStdString();
    } else {
      CefString(&settings->cache_path) = cache_path_.toStdString();
    }
  }

  // TODO(cretz): Due to a CEF bug, we cannot have user preferences persisted
  //  if we have a cache path. This is due to the fact that the pref JSON is
  //  created on the wrong thread.
  // See: https://bitbucket.org/chromiumembedded/cef/issues/2233
  if (settings->cache_path.length > 0 &&
      settings->persist_user_preferences == 1) {
    qWarning() <<
      "Unable to have cache path and user pref persistence set "
      "for a bubble due to internal bug. User pref persistence "
      "will be disabled";
    settings->persist_user_preferences = 0;
  }
}

CefRefPtr<CefRequestContext> Bubble::CreateCefRequestContext() const {
  CefRequestContextSettings settings;
  ApplyCefRequestContextSettings(&settings);
  return CefRequestContext::CreateContext(settings, nullptr);
}

bool Bubble::operator==(const Bubble& other) const {
  return name_ == other.name_ &&
      icon_path_ == other.icon_path_ &&
      icon_path_.isNull() == other.icon_path_.isNull() &&
      icon_color_ == other.icon_color_ &&
      cache_path_ == other.cache_path_ &&
      cache_path_.isNull() == other.cache_path_.isNull() &&
      browser_settings_ == other.browser_settings_ &&
      overrides_blocker_lists_ == other.overrides_blocker_lists_ &&
      enabled_blocker_list_ids_ == other.enabled_blocker_list_ids_;
}

bool Bubble::PersistBrowserSettings(
    const QHash<BrowserSetting::SettingKey, bool>& browser_settings,
    QVariant bubble_id) {
  QSqlQuery query;
  QString sql = "DELETE FROM browser_setting";
  if (bubble_id.isNull()) {
    sql += " WHERE bubble_id IS NULL";
  } else {
    sql += QString(" WHERE bubble_id = %1").arg(bubble_id.toString());
  }
  if (!Sql::Exec(&query, sql)) return false;
  if (browser_settings.isEmpty()) return true;
  QStringList values;
  for (auto key : browser_settings.keys()) {
    values << QString("(%1, '%2', %3)").
              arg(bubble_id.isNull() ? "NULL" : bubble_id.toString()).
              arg(BrowserSetting::KeyToString(key)).
              arg(browser_settings[key] ? 1 : 0);
  }
  return Sql::Exec(&query,
                   QString("INSERT INTO browser_setting "
                           "(bubble_id, key, value) VALUES %1").
                   arg(values.join(',')));
}

QHash<BrowserSetting::SettingKey, bool> Bubble::LoadBrowserSettings(
    QVariant bubble_id) {
  QHash<BrowserSetting::SettingKey, bool> ret;
  QSqlQuery query;
  QString sql = "SELECT * FROM browser_setting";
  if (bubble_id.isNull()) {
    sql += " WHERE bubble_id IS NULL";
  } else {
    sql += QString(" WHERE bubble_id = %1").arg(bubble_id.toString());
  }
  if (!Sql::Exec(&query, sql)) return ret;
  while (query.next()) {
    ret[BrowserSetting::QStringToKey(query.value("key").toString())] =
        query.value("value").toBool();
  }
  return ret;
}

bool Bubble::PersistEnabledBlockerListIds(
    const QSet<qlonglong>& enabled_blocker_list_ids,
    QVariant bubble_id) {
  QSqlQuery query;
  QString sql = "DELETE FROM enabled_blocker_list";
  if (bubble_id.isNull()) {
    sql += " WHERE bubble_id IS NULL";
  } else {
    sql += QString(" WHERE bubble_id = %1").arg(bubble_id.toString());
  }
  if (!Sql::Exec(&query, sql)) return false;
  if (enabled_blocker_list_ids.isEmpty()) return true;
  QStringList values;
  for (auto id : enabled_blocker_list_ids) {
    values << QString("(%1, %2)").
              arg(bubble_id.isNull() ? "NULL" : bubble_id.toString()).
              arg(id);
  }
  return Sql::Exec(&query,
                   QString("INSERT INTO enabled_blocker_list "
                           "(bubble_id, blocker_list_id) VALUES %1").
                   arg(values.join(',')));
}

QSet<qlonglong> Bubble::LoadEnabledBlockerListIds(QVariant bubble_id) {
  QSet<qlonglong> ret;
  QSqlQuery query;
  QString sql = "SELECT * FROM enabled_blocker_list";
  if (bubble_id.isNull()) {
    sql += " WHERE bubble_id IS NULL";
  } else {
    sql += QString(" WHERE bubble_id = %1").arg(bubble_id.toString());
  }
  if (!Sql::Exec(&query, sql)) return ret;
  while (query.next()) {
    ret << query.value("blocker_list_id").toLongLong();
  }
  return ret;
}

void Bubble::ApplyBrowserSettings(
    const QHash<BrowserSetting::SettingKey, bool>& source,
    CefBrowserSettings* target) {
  auto state = [&source](cef_state_t& state, BrowserSetting::SettingKey key) {
    if (source.contains(key)) {
      state = source[key] ? STATE_ENABLED : STATE_DISABLED;
    }
  };

  state(target->application_cache,
        BrowserSetting::ApplicationCache);
  state(target->databases,
        BrowserSetting::Databases);
  state(target->file_access_from_file_urls,
        BrowserSetting::FileAccessFromFileUrls);
  state(target->image_loading,
        BrowserSetting::ImageLoading);
  state(target->image_shrink_standalone_to_fit,
        BrowserSetting::ImageShrinkStandaloneToFit);
  state(target->javascript,
        BrowserSetting::JavaScript);
  state(target->javascript_access_clipboard,
        BrowserSetting::JavaScriptAccessClipboard);
  state(target->javascript_dom_paste,
        BrowserSetting::JavaScriptDomPaste);
  state(target->local_storage,
        BrowserSetting::LocalStorage);
  state(target->plugins,
        BrowserSetting::Plugins);
  state(target->remote_fonts,
        BrowserSetting::RemoteFonts);
  state(target->tab_to_links,
        BrowserSetting::TabToLinks);
  state(target->text_area_resize,
        BrowserSetting::TextAreaResize);
  state(target->universal_access_from_file_urls,
        BrowserSetting::UniversalAccessFromFileUrls);
  state(target->web_security,
        BrowserSetting::WebSecurity);
  state(target->webgl,
        BrowserSetting::WebGl);
}

void Bubble::ApplyBrowserSettings(
    const QHash<BrowserSetting::SettingKey, bool>& source,
    CefRequestContextSettings* target) {
  if (source.value(BrowserSetting::PersistUserPreferences, false)) {
    target->persist_user_preferences = 1;
  }
}

void Bubble::ApplyBrowserSettings(
    const QHash<BrowserSetting::SettingKey, bool>& source,
    CefSettings* target) {
  if (source.value(BrowserSetting::PersistUserPreferences, false)) {
    target->persist_user_preferences = 1;
  }
}

Bubble::Bubble(const QSqlRecord& record) {
  ApplySqlRecord(record);
}

void Bubble::ApplySqlRecord(const QSqlRecord& record) {
  id_ = record.value("id").toLongLong();
  order_index_ = record.value("order_index").toInt();
  name_ = record.value("name").toString();
  cache_path_ = record.value("cache_path").toString();
  icon_path_ = record.value("icon_path").toString();
  icon_color_ = QColor(record.value("icon_color").toString());
  browser_settings_ = LoadBrowserSettings(id_);
  enabled_blocker_list_ids_ = LoadEnabledBlockerListIds(id_);
  RebuildIcon();
}

void Bubble::RebuildIcon() {
  // Can only do this if there is an application
  if (!QCoreApplication::instance()) return;
  if (icon_path_.isNull()) {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    icon_ = QIcon(pixmap);
  } else {
    auto path = icon_path_;
    if (path.isEmpty()) path = ":/res/images/fontawesome/circle.png";
    if (icon_color_.isValid()) {
      icon_ = QIcon(*Util::CachedPixmapColorOverlay(path, QColor(icon_color_)));
    } else {
      icon_ = Util::CachedIcon(path);
    }
  }
}

QList<Bubble> Bubble::cached_bubbles_;

}  // namespace doogie
