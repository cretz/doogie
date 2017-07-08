#ifndef DOOGIE_BROWSERWIDGET_H_
#define DOOGIE_BROWSERWIDGET_H_

#include <QtWidgets>
#include "cef_widget.h"

class BrowserWidget : public QWidget {
  Q_OBJECT
 public:
  explicit BrowserWidget(Cef *cef,
                         const QString &url = "",
                         QWidget *parent = nullptr);

  void FocusUrlEdit();
  void FocusBrowser();

  // Matches CEF's numbering, don't change
  enum WindowOpenType {
    OpenTypeUnknown,
    OpenTypeCurrentTab,
    OpenTypeSingletonTab,
    OpenTypeNewForegroundTab,
    OpenTypeNewBackgroundTab,
    OpenTypeNewPopup,
    OpenTypeNewWindow,
    OpenTypeSaveToDisk,
    OpenTypeOffTheRecord,
    OpenTypeIgnoreAction
  };
  Q_ENUM(WindowOpenType)

 protected:
  void moveEvent(QMoveEvent *event);
  void resizeEvent(QResizeEvent *event);

 private:
  Cef *cef_;
  QLineEdit *url_edit_;
  CefWidget *cef_widg_;
  QStatusBar *status_bar_;

  void UpdateStatusBarLocation();

 signals:
  void TitleChanged(const QString &title);
  void FaviconChanged(const QIcon &icon);
  void TabOpen(WindowOpenType type, const QString &url, bool user_gesture);
};

#endif // DOOGIE_BROWSERWIDGET_H_
