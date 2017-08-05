#ifndef DOOGIE_BUBBLE_H_
#define DOOGIE_BUBBLE_H_

#include <QtWidgets>

#include "cef.h"
#include "profile.h"

namespace doogie {

class BubbleSettingsDialog;
class ProfileSettingsDialog;

class Bubble : public QObject {
  Q_OBJECT

 public:
  ~Bubble();
  QString Name() const;
  QString FriendlyName() const;
  void ApplyCefBrowserSettings(CefBrowserSettings* settings);
  void ApplyCefRequestContextSettings(CefRequestContextSettings* settings);
  CefRefPtr<CefRequestContext> CreateCefRequestContext();
  QIcon Icon();
  void InvalidateIcon();

 private:
  explicit Bubble(QJsonObject prefs, QObject* parent = nullptr);

  QJsonObject prefs_;
  QIcon cached_icon_;

  friend class BubbleSettingsDialog;
  friend class Profile;
  friend class ProfileSettingsDialog;
};
\
}  // namespace doogie

#endif  // DOOGIE_BUBBLE_H_
