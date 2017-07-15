#include "browser_stack.h"

BrowserStack::BrowserStack(Cef *cef, QWidget *parent)
    : QStackedWidget(parent), cef_(cef) {
  connect(this, &BrowserStack::currentChanged, [this](int) {
    emit BrowserChanged(CurrentBrowser());
    emit CurrentBrowserOrLoadingStateChanged();
  });
}

QPointer<BrowserWidget> BrowserStack::NewBrowser(const QString &url) {
  auto widg = new BrowserWidget(cef_, url);
  connect(widg, &BrowserWidget::LoadingStateChanged, [this, widg]() {
    if (currentWidget() == widg) emit CurrentBrowserOrLoadingStateChanged();
  });
  addWidget(widg);
  return widg;
}

BrowserWidget* BrowserStack::CurrentBrowser() {
  return (BrowserWidget*) currentWidget();
}
