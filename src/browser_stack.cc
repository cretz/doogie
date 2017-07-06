#include "browser_stack.h"

BrowserStack::BrowserStack(Cef *cef, QWidget *parent)
    : QStackedWidget(parent), cef_(cef) {

}

QPointer<BrowserWidget> BrowserStack::NewBrowser() {
  auto widg = new BrowserWidget(cef_);
  addWidget(widg);
  return widg;
}
