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

  static const QString kInMemoryPath;

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
  static QKeySequence KeySequenceOrEmpty(const QString& str);

  QString FriendlyName() const;
  QString Path() const;
  bool InMemory() const;

  void ApplyCefSettings(CefSettings* settings);
  void ApplyCefBrowserSettings(CefBrowserSettings* settings);

  const QList<Bubble*> Bubbles() const;
  Bubble* DefaultBubble() const;
  Bubble* BubbleByName(const QString& name) const;
  void AddBubble(Bubble* bubble);
  bool SavePrefs();

  void ApplyActionShortcuts();

 private:
  static const QString kAppDataPath;

  explicit Profile(const QString& path,
                   QJsonObject prefs,
                   QObject* parent = nullptr);
  static void SetCurrent(Profile* profile);

  static Profile* current_;
  static QList<BrowserSetting> browser_settings_;
  QString path_;
  QJsonObject prefs_;
  QList<Bubble*> bubbles_;

  friend class ProfileSettingsDialog;
};
\
}  // namespace doogie

#endif  // DOOGIE_PROFILE_H_
