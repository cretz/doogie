#ifndef DOOGIE_BROWSER_STACK_H_
#define DOOGIE_BROWSER_STACK_H_

#include <QtWidgets>

#include "browser_widget.h"
#include "bubble.h"

namespace doogie {

class BrowserStack : public QStackedWidget {
  Q_OBJECT

 public:
  explicit BrowserStack(const Cef& cef, QWidget* parent = nullptr);
  BrowserWidget* NewBrowser(Bubble* bubble,
                            const QString& url);
  BrowserWidget* CurrentBrowser() const;
  QList<BrowserWidget*> Browsers() const;

 signals:
  void BrowserChanged(BrowserWidget* browser);
  void CurrentBrowserOrLoadingStateChanged();
  void ShowDevToolsRequest(BrowserWidget* browser, const QPoint& inspect_at);
  void BrowserCloseCancelled(BrowserWidget* browser);

 private:
  void SetupActions();

  const Cef& cef_;
};

}  // namespace doogie

#endif  // DOOGIE_BROWSER_STACK_H_
