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
  QIcon CurrentFavicon();
  QString CurrentTitle();
  bool Loading();
  bool CanGoBack();
  bool CanGoForward();

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
  QToolButton* back_button_;
  QToolButton* forward_button_;
  QMenu* nav_menu_;
  QLineEdit *url_edit_;
  QToolButton* refresh_button_;
  QToolButton* stop_button_;
  CefWidget *cef_widg_;
  QStatusBar *status_bar_;
  QIcon current_favicon_;
  QString current_title_;
  bool loading_ = false;
  bool can_go_back_ = false;
  bool can_go_forward_ = false;

  void UpdateStatusBarLocation();
  void RebuildNavMenu();

 signals:
  void TitleChanged();
  void FaviconChanged();
  void LoadingStateChanged();
  void PageOpen(WindowOpenType type, const QString &url, bool user_gesture);
};

#endif // DOOGIE_BROWSERWIDGET_H_
