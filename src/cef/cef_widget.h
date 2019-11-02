#ifndef DOOGIE_CEF_WIDGET_H_
#define DOOGIE_CEF_WIDGET_H_

#include <QtWidgets>
#include <vector>

#include "bubble.h"
#include "cef/cef_base_widget.h"
#include "cef/cef_handler.h"

namespace doogie {

// Normal browser widget wrapping CEF.
class CefWidget : public CefBaseWidget {
  Q_OBJECT

 public:
  struct NavEntry {
    QString url;
    QString title;
    bool current;
  };

  explicit CefWidget(const Cef& cef,
                     const Bubble& bubble,
                     const QString& url = "",
                     QWidget* parent = nullptr,
                     const QSize& initial_size = QSize());
  ~CefWidget();

  std::vector<NavEntry> NavEntries() const;
  CefRefPtr<CefSSLStatus> CurrentSSLStatus() const;
  void LoadUrl(const QString& url);
  // No frame means main frame
  void ShowStringPage(const QString& data,
                      const QString& mime_type,
                      CefRefPtr<CefFrame> frame = nullptr);
  QString CurrentUrl() const;
  bool HasDocument() const;
  void TryClose();

  void LoadFaviconIcon(const QString& url);

  // Basically just calls history.go
  void Go(int num);
  void Refresh(bool ignore_cache);
  void Stop();
  void Print();

  void Find(const QString& text,
            bool forward,
            bool match_case,
            bool continued);
  void CancelFind(bool clear_selection);

  void ExecJs(const QString& js);

  void ShowDevTools(CefBaseWidget* widg, const QPoint& inspect_at);
  void ExecDevToolsJs(const QString& js);
  void CloseDevTools();

  double ZoomLevel() const;
  void SetZoomLevel(double level);

  void SetJsDialogCallback(CefHandler::JsDialogCallback callback);
  void SetResourceLoadCallback(CefHandler::ResourceLoadCallback callback);

  void ApplyFullscreen(bool fullscreen);

 signals:
  void Closed();
  void PreContextMenu(CefRefPtr<CefContextMenuParams> params,
                      CefRefPtr<CefMenuModel> model);
  void ContextMenuCommand(CefRefPtr<CefContextMenuParams> params,
                          int command_id,
                          CefContextMenuHandler::EventFlags event_flags);
  void UrlChanged(const QString& url);
  void TitleChanged(const QString& title);
  void StatusChanged(const QString& status);
  void FaviconChanged(const QString& url, const QIcon& icon);
  void DownloadRequested(const Download& download,
                         CefRefPtr<CefBeforeDownloadCallback> callback);
  void DownloadUpdated(const Download& download);
  void LoadStateChanged(bool is_loading,
                        bool can_go_back,
                        bool can_go_forward);
  void LoadStart(CefLoadHandler::TransitionType transition_type);
  void LoadError(CefRefPtr<CefFrame> frame,
                 CefLoadHandler::ErrorCode error_code,
                 const QString& error_text,
                 const QString& failed_url);
  void CertificateError(
      cef_errorcode_t cert_error,
      const QString& request_url,
      CefRefPtr<CefSSLInfo> ssl_info,
      CefRefPtr<CefRequestCallback> callback);
  void PageOpen(CefHandler::WindowOpenType type,
                const QString& url,
                bool user_gesture);
  void DevToolsLoadComplete();
  void DevToolsClosed();
  void FindResult(int count, int index);
  void ShowBeforeUnloadDialog(const QString& message_text,
                              bool is_reload,
                              CefRefPtr<CefJSDialogCallback> callback);

 protected:
  void focusInEvent(QFocusEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void UpdateSize() override;

 private:
  class FaviconDownloadCallback : public CefDownloadImageCallback {
   public:
    explicit FaviconDownloadCallback(QPointer<CefWidget> cef_widg)
      : cef_widg_(cef_widg) {}
    void OnDownloadImageFinished(const CefString& image_url,
                                 int http_status_code,
                                 CefRefPtr<CefImage> image) override;
   private:
    QPointer<CefWidget> cef_widg_;
    IMPLEMENT_REFCOUNTING(FaviconDownloadCallback);
    DISALLOW_COPY_AND_ASSIGN(FaviconDownloadCallback);
  };

  class NavEntryVisitor : public CefNavigationEntryVisitor {
   public:
    bool Visit(CefRefPtr<CefNavigationEntry> entry,
               bool current, int, int) override;
    std::vector<NavEntry> Entries() const { return entries_; }

   private:
    std::vector<NavEntry> entries_;
    IMPLEMENT_REFCOUNTING(NavEntryVisitor);
  };

  class NavEntryCurrentSslVisitor : public CefNavigationEntryVisitor {
   public:
    bool Visit(CefRefPtr<CefNavigationEntry> entry,
               bool current, int, int) override;
    CefRefPtr<CefSSLStatus> SslStatus() const { return ssl_status_; }

   private:
    CefRefPtr<CefSSLStatus> ssl_status_;
    IMPLEMENT_REFCOUNTING(NavEntryCurrentSslVisitor);
  };

  void InitBrowser(const Bubble& bubble, const QString& url);

  void AuthRequest(const QString& realm,
                   CefRefPtr<CefAuthCallback> callback) const;

  CefRefPtr<CefHandler> handler_;
  CefRefPtr<CefBrowser> browser_;
  CefRefPtr<CefHandler> dev_tools_handler_;
  CefRefPtr<CefBrowser> dev_tools_browser_;
  bool download_favicon_ = false;
  bool js_triggered_fullscreen_ = false;
};

}  // namespace doogie

#endif  // DOOGIE_CEF_WIDGET_H_
