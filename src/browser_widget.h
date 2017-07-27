#ifndef DOOGIE_BROWSER_WIDGET_H_
#define DOOGIE_BROWSER_WIDGET_H_

#include <QtWidgets>
#include "cef_widget.h"
#include "find_widget.h"

namespace doogie {

class BrowserWidget : public QWidget {
  Q_OBJECT

 public:
  enum ContextMenuCommand {
    ContextMenuOpenLinkChildPage = MENU_ID_USER_FIRST,
    ContextMenuOpenLinkChildPageBackground,
    ContextMenuOpenLinkTopLevelPage,
    ContextMenuCopyLinkAddress,
    ContextMenuInspectElement,
    ContextMenuViewPageSource,
    ContextMenuViewFrameSource
  };
  Q_ENUM(ContextMenuCommand)

  explicit BrowserWidget(Cef* cef,
                         const QString& url = "",
                         QWidget* parent = nullptr);

  void LoadUrl(const QString& url);
  void TryClose();
  void FocusUrlEdit();
  void FocusBrowser();
  QIcon CurrentFavicon();
  QString CurrentTitle();
  QString CurrentUrl();
  bool Loading();
  bool CanGoBack();
  bool CanGoForward();

  void Refresh();
  void Stop();
  void Back();
  void Forward();
  void Print();
  void ShowFind();
  void ExecJs(const QString& js);

  void ShowDevTools(CefBaseWidget* widg, const QPoint& inspect_at);
  void ExecDevToolsJs(const QString& js);
  void CloseDevTools();

  double GetZoomLevel();
  void SetZoomLevel(double level);

  bool Suspended();
  void SetSuspended(bool suspend);

  QJsonObject DebugDump();

 signals:
  void TitleChanged();
  void FaviconChanged();
  void LoadingStateChanged();
  void PageOpen(CefHandler::WindowOpenType type,
                const QString& url,
                bool user_gesture);
  void DevToolsLoadComplete();
  void DevToolsClosed();
  void FindResult(int count, int index);
  void ShowDevToolsRequest(const QPoint& inspect_at);
  void AboutToShowJSDialog();
  void CloseCancelled();
  void SuspensionChanged();

 protected:
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  void RecreateCefWidget(const QString& url);
  void ShowAsSuspendedScreenshot();
  void UpdateStatusBarLocation();
  void RebuildNavMenu();
  void BuildContextMenu(CefRefPtr<CefContextMenuParams> params,
                        CefRefPtr<CefMenuModel> model);
  void HandleContextMenuCommand(CefRefPtr<CefContextMenuParams> params,
                                int command_id,
                                CefContextMenuHandler::EventFlags event_flags);

  Cef* cef_;
  QToolButton* back_button_ = nullptr;
  QToolButton* forward_button_ = nullptr;
  QMenu* nav_menu_ = nullptr;
  QLineEdit* url_edit_ = nullptr;
  QToolButton* refresh_button_ = nullptr;
  QToolButton* stop_button_ = nullptr;
  CefWidget* cef_widg_ = nullptr;
  FindWidget* find_widg_ = nullptr;
  QLabel* status_bar_ = nullptr;
  QIcon current_favicon_;
  QString current_title_;
  bool loading_ = false;
  bool can_go_back_ = false;
  bool can_go_forward_ = false;
  bool suspended_ = false;
  QString suspended_url_;
  QPixmap suspended_screenshot_;
};

}  // namespace doogie

#endif  // DOOGIE_BROWSER_WIDGET_H_
