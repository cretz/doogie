#include "browser_stack.h"

namespace doogie {

BrowserStack::BrowserStack(Cef* cef, QWidget* parent)
    : QStackedWidget(parent), cef_(cef) {
  connect(this, &BrowserStack::currentChanged, [this](int) {
    emit BrowserChanged(CurrentBrowser());
    emit CurrentBrowserOrLoadingStateChanged();
  });
}

QPointer<BrowserWidget> BrowserStack::NewBrowser(const QString& url) {
  auto widg = new BrowserWidget(cef_, "");
  connect(widg, &BrowserWidget::LoadingStateChanged, [this, widg]() {
    if (currentWidget() == widg) emit CurrentBrowserOrLoadingStateChanged();
  });
  connect(widg, &BrowserWidget::ShowDevToolsRequest,
          [this, widg](const QPoint& inspect_at) {
    emit ShowDevToolsRequest(widg, inspect_at);
  });
  connect(widg, &BrowserWidget::CloseCancelled, [this, widg]() {
    emit BrowserCloseCancelled(widg);
  });
  // We load the URL separately so we can have the loading icon and what not
  if (!url.isEmpty()) widg->LoadUrl(url);
  addWidget(widg);
  return widg;
}

BrowserWidget* BrowserStack::CurrentBrowser() {
  return static_cast<BrowserWidget*>(currentWidget());
}

}  // namespace doogie
