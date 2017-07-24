#include "browser_stack.h"
#include "action_manager.h"

namespace doogie {

BrowserStack::BrowserStack(Cef* cef, QWidget* parent)
    : QStackedWidget(parent), cef_(cef) {
  connect(this, &BrowserStack::currentChanged, [this](int) {
    emit BrowserChanged(CurrentBrowser());
    emit CurrentBrowserOrLoadingStateChanged();
  });
  SetupActions();
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

void BrowserStack::SetupActions() {
  connect(ActionManager::Action(ActionManager::FocusAddressBar),
          &QAction::triggered, [this]() {
    auto browser = CurrentBrowser();
    if (browser) browser->FocusUrlEdit();
  });
  connect(ActionManager::Action(ActionManager::FocusBrowser),
          &QAction::triggered, [this]() {
    auto browser = CurrentBrowser();
    if (browser) browser->FocusBrowser();
  });

  // Let's disable/enable a bunch of actions on change
  connect(this, &BrowserStack::CurrentBrowserOrLoadingStateChanged, [this]() {
    auto current = CurrentBrowser();
    auto enabled = [](ActionManager::Type type, bool value) {
      ActionManager::Action(type)->setEnabled(value);
    };
    enabled(ActionManager::NewChildForegroundPage, current);
    enabled(ActionManager::NewChildBackgroundPage, current);
    enabled(ActionManager::Reload, current && !current->Loading());
    enabled(ActionManager::Stop, current && current->Loading());
    enabled(ActionManager::Back, current && current->CanGoBack());
    enabled(ActionManager::Forward, current && current->CanGoForward());
    enabled(ActionManager::Print, current);
    enabled(ActionManager::ZoomIn, current);
    enabled(ActionManager::ZoomOut, current);
    enabled(ActionManager::ResetZoom, current);
    enabled(ActionManager::FindInPage, current);
  });
  emit CurrentBrowserOrLoadingStateChanged();
}

}  // namespace doogie
