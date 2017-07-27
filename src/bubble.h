#ifndef DOOGIE_BUBBLE_H_
#define DOOGIE_BUBBLE_H_

#include <QtWidgets>
#include "cef.h"
#include "profile.h"

namespace doogie {

class Bubble : public QObject {
  Q_OBJECT

 public:
  QString Name();
  CefBrowserSettings CreateCefBrowserSettings();

 private:
  explicit Bubble(QJsonObject prefs, QObject* parent = nullptr);

  QJsonObject prefs_;

  friend class Profile;
};
\
}  // namespace doogie

#endif  // DOOGIE_BUBBLE_H_
