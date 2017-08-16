#ifndef DOOGIE_BROWSER_WIDGET_H_
#define DOOGIE_BROWSER_WIDGET_H_

#include <QtWidgets>

#include "bubble.h"
#include "cef/cef_widget.h"
#include "find_widget.h"
#include "url_edit.h"

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

  explicit BrowserWidget(const Cef& cef,
                         const Bubble& bubble,
                         const QString& url = "",
                         QWidget* parent = nullptr);

  void LoadUrl(const QString& url);
  void SetUrlText(const QString& url);
  void TryClose();
  void FocusUrlEdit();
  void FocusBrowser();
  const Bubble& CurrentBubble() const;
  void ChangeCurrentBubble(const Bubble& bubble);
  QIcon CurrentFavicon() const;
  QString CurrentTitle() const;
  QString CurrentUrl() const;
  bool Loading() const;
  bool CanGoBack() const;
  bool CanGoForward() const;

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

  double ZoomLevel() const;
  void SetZoomLevel(double level);

  bool Suspended() const;
  void SetSuspended(bool suspend, const QString& url_override = QString());

  QJsonObject DebugDump() const;

 signals:
  void TitleChanged();
  void FaviconChanged();
  void DownloadRequested(const Download& download,
                         CefRefPtr<CefBeforeDownloadCallback> callback);
  void DownloadUpdated(const Download& download);
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
  void BubbleMaybeChanged();

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

  const Cef& cef_;
  Bubble bubble_;
  QToolButton* back_button_ = nullptr;
  QToolButton* forward_button_ = nullptr;
  QMenu* nav_menu_ = nullptr;
  UrlEdit* url_edit_ = nullptr;
  QToolButton* refresh_button_ = nullptr;
  QToolButton* stop_button_ = nullptr;
  CefWidget* cef_widg_ = nullptr;
  FindWidget* find_widg_ = nullptr;
  QLabel* status_bar_ = nullptr;
  QString current_favicon_url_;
  QIcon current_favicon_;
  QString current_title_;
  bool loading_ = false;
  bool can_go_back_ = false;
  bool can_go_forward_ = false;
  bool suspended_ = false;
  QString suspended_url_;
  QPixmap suspended_screenshot_;
  bool next_load_is_error_ = true;
};

}  // namespace doogie

#endif  // DOOGIE_BROWSER_WIDGET_H_
