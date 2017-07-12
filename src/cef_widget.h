#ifndef DOOGIE_CEFWIDGET_H_
#define DOOGIE_CEFWIDGET_H_

#include <QtWidgets>
#include "cef_base_widget.h"
#include "cef_handler.h"

class CefWidget : public CefBaseWidget {
  Q_OBJECT
 public:
  CefWidget(Cef *cef, const QString &url = "", QWidget *parent = nullptr);
  ~CefWidget();

  // If result is non-null, it needs to replace this widget
  QPointer<QWidget> OverrideWidget();\
  void LoadUrl(const QString &url);

  // Basically just calls history.go
  void Go(int num);
  void Refresh(bool ignore_cache);
  void Stop();

  struct NavEntry {
    QString url;
    QString title;
    bool current;
  };
  std::vector<NavEntry> NavEntries();

 protected:
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  void UpdateSize() override;

 private:
  CefRefPtr<CefHandler> handler_;
  CefRefPtr<CefBrowser> browser_;
  QPointer<QWidget> override_widget_;
  bool download_favicon_;

  void InitBrowser(const QString &url);

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
               bool current, int, int) override {
      NavEntry nav_entry = {
        QString::fromStdString(entry->GetURL().ToString()),
        QString::fromStdString(entry->GetTitle().ToString()),
        current
      };
      entries_.push_back(nav_entry);
      return true;
    }
    std::vector<NavEntry> Entries() { return entries_; }
   private:
    std::vector<NavEntry> entries_;
    IMPLEMENT_REFCOUNTING(NavEntryVisitor);
  };

 signals:
  void UrlChanged(const QString &url);
  void TitleChanged(const QString &title);
  void StatusChanged(const QString &status);
  void FaviconChanged(const QIcon &icon);
  void LoadStateChanged(bool is_loading,
                        bool can_go_back,
                        bool can_go_forward);
  void PageOpen(CefRequestHandler::WindowOpenDisposition type,
                const QString &url,
                bool user_gesture);
};

#endif // DOOGIE_CEFWIDGET_H_
