#ifndef DOOGIE_BROWSER_STACK_H_
#define DOOGIE_BROWSER_STACK_H_

#include <QtWidgets>
#include "browser_widget.h"

namespace doogie {

class BrowserStack : public QStackedWidget {
  Q_OBJECT

 public:
  explicit BrowserStack(Cef* cef, QWidget* parent = nullptr);
  QPointer<BrowserWidget> NewBrowser(const QString& url);
  BrowserWidget* CurrentBrowser();

 signals:
  void BrowserChanged(BrowserWidget* browser);
  void CurrentBrowserOrLoadingStateChanged();
  void ShowDevToolsRequest(BrowserWidget* browser, const QPoint& inspect_at);

 private:
  Cef* cef_;
};

}  // namespace doogie

#endif  // DOOGIE_BROWSER_STACK_H_
