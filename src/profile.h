#ifndef DOOGIE_PROFILE_H_
#define DOOGIE_PROFILE_H_

#include <QtWidgets>
#include "cef.h"

namespace doogie {

class Bubble;

class Profile : public QObject {
  Q_OBJECT

 public:
  static Profile* Current();
  // False on failure. As a special case, an empty
  // or null string creates an in-mem-only profile
  static bool CreateProfile(const QString& path);
  // False on failure
  static bool LoadProfile(const QString& path);
  // False on failure
  static bool LoadProfileFromCommandLine(int argc, char* argv[]);

  QString FriendlyPath();

  CefSettings CreateCefSettings();
  CefBrowserSettings CreateBrowserSettings();

  const QList<Bubble*> Bubbles();
  Bubble* DefaultBubble();
  // If name is empty, a special "default" bubble is returned
  Bubble* BubbleByName(const QString& name);
  bool SavePrefs();

  QString ShowChangeProfileDialog(bool& wants_restart);

 private:
  explicit Profile(const QString& path,
                   QJsonObject prefs,
                   QObject* parent = nullptr);
  static void SetCurrent(Profile* profile);
  static QString FriendlyPath(const QString& path);

  static Profile* current_;
  static const QString kAppDataPath;
  QString path_;
  QJsonObject prefs_;
  QList<Bubble*> bubbles_;
};
\
}  // namespace doogie

#endif  // DOOGIE_PROFILE_H_
