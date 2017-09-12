#ifndef DOOGIE_BROWSER_SETTING_H_
#define DOOGIE_BROWSER_SETTING_H_

#include <QtWidgets>

namespace doogie {

// Metadata model for a browser setting. Doesn't store the actual setting,
// just has a set of static methods for getting the ones available.
class BrowserSetting {
  Q_GADGET

 public:
  enum SettingKey {
    ApplicationCache,
    Databases,
    EnableNetSecurityExpiration,
    FileAccessFromFileUrls,
    ImageLoading,
    ImageShrinkStandaloneToFit,
    JavaScript,
    JavaScriptAccessClipboard,
    JavaScriptDomPaste,
    JavaScriptOpenWindows,
    LocalStorage,
    PersistUserPreferences,
    Plugins,
    RemoteFonts,
    TabToLinks,
    TextAreaResize,
    UniversalAccessFromFileUrls,
    WebGl,
    WebSecurity
  };
  Q_ENUM(SettingKey)

  static const QList<BrowserSetting> kSettings;

  static const char* KeyToString(SettingKey key) {
    const auto meta = QMetaEnum::fromType<SettingKey>();
    return meta.valueToKey(key);
  }
  static SettingKey StringToKey(const char* key, bool* ok = nullptr) {
    const auto meta = QMetaEnum::fromType<SettingKey>();
    return static_cast<SettingKey>(meta.keyToValue(key, ok));
  }
  static QString KeyToQString(SettingKey key) {
    return QString(KeyToString(key));
  }
  static SettingKey QStringToKey(const QString& key, bool* ok = nullptr) {
    return StringToKey(qPrintable(key), ok);
  }

  BrowserSetting(SettingKey key,
                 const QString& name,
                 const QString& desc);

  SettingKey Key() const { return key_; }
  QString Name() const { return name_; }
  QString Desc() const { return desc_; }

 private:
  SettingKey key_;
  QString name_;
  QString desc_;
};

}  // namespace doogie

#endif  // DOOGIE_BROWSER_SETTING_H_
