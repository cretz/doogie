#ifndef DOOGIE_BROWSER_STACK_H_
#define DOOGIE_BROWSER_STACK_H_

#include <QtWidgets>

#include "browser_widget.h"
#include "bubble.h"

namespace doogie {

// Main stack widget holding the different browsers.
class BrowserStack : public QStackedWidget {
  Q_OBJECT

 public:
  explicit BrowserStack(const Cef& cef, QWidget* parent = nullptr);
  BrowserWidget* NewBrowser(const Bubble& bubble,
                            const QString& url);
  BrowserWidget* CurrentBrowser() const;
  QList<BrowserWidget*> Browsers() const;

  void SetResourceLoadCallback(BrowserWidget::ResourceLoadCallback callback);

 signals:
  void BrowserChanged(BrowserWidget* browser);
  void BrowserDestroyed(BrowserWidget* browser);
  void CurrentBrowserOrLoadingStateChanged();
  void ShowDevToolsRequest(BrowserWidget* browser, const QPoint& inspect_at);
  void BrowserCloseCancelled(BrowserWidget* browser);
  void DownloadRequested(const Download& download,
                         CefRefPtr<CefBeforeDownloadCallback> callback);
  void DownloadUpdated(const Download& download);

 private:
  void SetupActions();

  const Cef& cef_;
  BrowserWidget::ResourceLoadCallback resource_load_callback_;
};

}  // namespace doogie

#endif  // DOOGIE_BROWSER_STACK_H_
