#include "browser_stack.h"

BrowserStack::BrowserStack(Cef *cef, QWidget *parent)
    : QStackedWidget(parent), cef_(cef) {

}

QPointer<BrowserWidget> BrowserStack::NewBrowser(const QString &url) {
  auto widg = new BrowserWidget(cef_, url);
  addWidget(widg);
  return widg;
}
