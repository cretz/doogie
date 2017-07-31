#ifndef DOOGIE_PROFILE_H_
#define DOOGIE_PROFILE_H_

#include <QtWidgets>
#include "cef.h"

namespace doogie {

class Bubble;
class ProfileSettingsDialog;

class Profile : public QObject {
  Q_OBJECT

 public:
  struct BrowserSetting {
    QString name;
    QString desc;
    QString field;
  };

  static Profile* Current();
  // False on failure
  static bool CreateOrLoadProfile(const QString& path,
                                  bool set_current = true);
  // False on failure. As a special case, an empty
  // or null string creates an in-mem-only profile
  static bool CreateProfile(const QString& path, bool set_current = true);
  // False on failure
  static bool LoadProfile(const QString& path, bool set_current = true);
  // False on failure
  static bool LoadProfileFromCommandLine(int argc, char* argv[]);
  // False on failure
  static bool LaunchWithProfile(const QString& profile_path);

  static QString FriendlyName(const QString& path);
  static QList<BrowserSetting> PossibleBrowserSettings();
  static QStringList LastTenProfilePaths();

  QString FriendlyName();
  QString Path();
  bool InMemory();

  CefSettings CreateCefSettings();
  CefBrowserSettings CreateBrowserSettings();

  QList<Bubble*> Bubbles();
  Bubble* DefaultBubble();
  // If name is empty, a special "default" bubble is returned
  Bubble* BubbleByName(const QString& name);
  bool SavePrefs();

  static const QString kInMemoryPath;

 private:
  explicit Profile(const QString& path,
                   QJsonObject prefs,
                   QObject* parent = nullptr);
  static void SetCurrent(Profile* profile);

  static Profile* current_;
  static const QString kAppDataPath;
  static QList<BrowserSetting> browser_settings_;
  QString path_;
  QJsonObject prefs_;
  QList<Bubble*> bubbles_;

  friend class ProfileSettingsDialog;
};
\
}  // namespace doogie

#endif  // DOOGIE_PROFILE_H_
