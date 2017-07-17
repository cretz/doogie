#ifndef DOOGIE_BROWSER_WIDGET_H_
#define DOOGIE_BROWSER_WIDGET_H_

#include <QtWidgets>
#include "cef_widget.h"
#include "find_widget.h"

namespace doogie {

class BrowserWidget : public QWidget {
  Q_OBJECT

 public:
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

  void FocusUrlEdit();
  void FocusBrowser();
  QIcon CurrentFavicon();
  QString CurrentTitle();
  bool Loading();
  bool CanGoBack();
  bool CanGoForward();

  void Refresh();
  void Stop();
  void Back();
  void Forward();
  void Print();
  void ShowFind();

  void ShowDevTools(CefBaseWidget* widg, const QPoint& inspect_at);
  void ExecDevToolsJs(const QString& js);
  void CloseDevTools();

  double GetZoomLevel();
  void SetZoomLevel(double level);

  QJsonObject DebugDump();

 signals:
  void TitleChanged();
  void FaviconChanged();
  void LoadingStateChanged();
  void PageOpen(WindowOpenType type, const QString& url, bool user_gesture);
  void DevToolsLoadComplete();
  void DevToolsClosed();
  void FindResult(int count, int index);
  void ShowDevToolsRequest(const QPoint& inspect_at);

 protected:
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  void UpdateStatusBarLocation();
  void RebuildNavMenu();
  void BuildContextMenu(CefRefPtr<CefContextMenuParams> params,
                        CefRefPtr<CefMenuModel> model);
  void HandleContextMenuCommand(CefRefPtr<CefContextMenuParams> params,
                                int command_id,
                                CefContextMenuHandler::EventFlags event_flags);

  Cef* cef_;
  QToolButton* back_button_;
  QToolButton* forward_button_;
  QMenu* nav_menu_;
  QLineEdit* url_edit_;
  QToolButton* refresh_button_;
  QToolButton* stop_button_;
  CefWidget* cef_widg_;
  FindWidget* find_widg_;
  QStatusBar* status_bar_;
  QIcon current_favicon_;
  QString current_title_;
  bool loading_ = false;
  bool can_go_back_ = false;
  bool can_go_forward_ = false;
};

}  // namespace doogie

#endif  // DOOGIE_BROWSER_WIDGET_H_
