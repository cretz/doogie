#ifndef DOOGIE_BROWSER_STACK_H_
#define DOOGIE_BROWSER_STACK_H_

#include <QtWidgets>
#include "browser_widget.h"
#include "bubble.h"

namespace doogie {

class BrowserStack : public QStackedWidget {
  Q_OBJECT

 public:
  explicit BrowserStack(Cef* cef, QWidget* parent = nullptr);
  QPointer<BrowserWidget> NewBrowser(Bubble* bubble, const QString& url);
  BrowserWidget* CurrentBrowser();
  QList<BrowserWidget*> Browsers();

 signals:
  void BrowserChanged(BrowserWidget* browser);
  void CurrentBrowserOrLoadingStateChanged();
  void ShowDevToolsRequest(BrowserWidget* browser, const QPoint& inspect_at);
  void BrowserCloseCancelled(BrowserWidget* browser);

 private:
  void SetupActions();

  Cef* cef_;
};

}  // namespace doogie

#endif  // DOOGIE_BROWSER_STACK_H_
