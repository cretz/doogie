#include "browser_setting.h"

namespace doogie {

const QList<BrowserSetting> BrowserSetting::kSettings = {
  BrowserSetting(
    BrowserSetting::ApplicationCache,
    "Application Cache?",
    "Controls whether the application cache can be used."),
  BrowserSetting(
    BrowserSetting::Databases,
    "Databases?",
    "Controls whether databases can be used."),
  BrowserSetting(
    BrowserSetting::EnableNetSecurityExpiration,
    "Enable Net Security Expiration?",
    "DEPRECATED: No longer takes effect."),
  BrowserSetting(
    BrowserSetting::FileAccessFromFileUrls,
    "File Access From File URLs?",
    "Controls whether file URLs will have access to other file URLs."),
  BrowserSetting(
    BrowserSetting::ImageLoading,
    "Image Loading?",
    "Controls whether image URLs will be loaded from the network."),
  BrowserSetting(
    BrowserSetting::ImageShrinkStandaloneToFit,
    "Image Fit To Page?",
    "Controls whether standalone images will be shrunk to fit the page."),
  BrowserSetting(
    BrowserSetting::JavaScript,
    "JavaScript?",
    "Controls whether JavaScript can be executed."),
  BrowserSetting(
    BrowserSetting::JavaScriptAccessClipboard,
    "JavaScript Clipboard Access?",
    "Controls whether JavaScript can access the clipboard."),
  BrowserSetting(
    BrowserSetting::JavaScriptDomPaste,
    "JavaScript DOM Paste?",
    "Controls whether DOM pasting is supported in the editor via "
    "execCommand(\"paste\"). Requires JavaScript clipboard access."),
  BrowserSetting(
    BrowserSetting::JavaScriptOpenWindows,
    "JavaScript Open Windows?",
    "Controls whether JavaScript can be used for opening windows."),
  BrowserSetting(
    BrowserSetting::LocalStorage,
    "Local Storage?",
    "Controls whether local storage can be used."),
  BrowserSetting(
    BrowserSetting::PersistUserPreferences,
    "Persist User Preferences?",
    "Whether to persist user preferences as a JSON file "
    "in the cache path. Requires cache to be enabled."),
  BrowserSetting(
    BrowserSetting::Plugins,
    "Browser Plugins?",
    "Controls whether any plugins will be loaded."),
  BrowserSetting(
    BrowserSetting::RemoteFonts,
    "Remote Fonts?",
    "Controls the loading of fonts from remote sources."),
  BrowserSetting(
    BrowserSetting::TabToLinks,
    "Tab to Links?",
    "Controls whether the tab key can advance focus to links."),
  BrowserSetting(
    BrowserSetting::TextAreaResize,
    "Text Area Resize?",
    "Controls whether text areas can be resized."),
  BrowserSetting(
    BrowserSetting::UniversalAccessFromFileUrls,
    "Universal Access From File URLs?",
    "Controls whether file URLs will have access to all URLs."),
  BrowserSetting(
    BrowserSetting::WebSecurity,
    "Web Security?",
    "Controls whether web security restrictions (same-origin policy) "
    "will be enforced."),
  BrowserSetting(
    BrowserSetting::WebGl,
    "WebGL?",
    "Controls whether WebGL can be used."),
};

BrowserSetting::BrowserSetting(SettingKey key,
                               const QString& name,
                               const QString& desc)
    : key_(key), name_(name), desc_(desc) {
}

}  // namespace doogie
