#include "browser_stack.h"

#include "action_manager.h"

namespace doogie {

BrowserStack::BrowserStack(const Cef& cef, QWidget* parent)
    : QStackedWidget(parent), cef_(cef) {
  connect(this, &BrowserStack::currentChanged, [=](int) {
    emit BrowserChanged(CurrentBrowser());
    emit CurrentBrowserOrLoadingStateChanged();
  });
  SetupActions();
}

BrowserWidget* BrowserStack::NewBrowser(const Bubble& bubble,
                                        const QString& url) {
  auto widg = new BrowserWidget(cef_, bubble, "", this);
  connect(widg, &BrowserWidget::LoadingStateChanged, [=]() {
    if (currentWidget() == widg) emit CurrentBrowserOrLoadingStateChanged();
  });
  connect(widg, &BrowserWidget::ShowDevToolsRequest,
          [=](const QPoint& inspect_at) {
    emit ShowDevToolsRequest(widg, inspect_at);
  });
  connect(widg, &BrowserWidget::CloseCancelled, [=]() {
    emit BrowserCloseCancelled(widg);
  });
  connect(widg, &BrowserWidget::DownloadRequested,
          this, &BrowserStack::DownloadRequested);
  connect(widg, &BrowserWidget::DownloadUpdated,
          this, &BrowserStack::DownloadUpdated);
  // We load the URL separately so we can have the loading icon and what not
  if (!url.isEmpty()) widg->LoadUrl(url);
  addWidget(widg);
  return widg;
}

BrowserWidget* BrowserStack::CurrentBrowser() const {
  return qobject_cast<BrowserWidget*>(currentWidget());
}

QList<BrowserWidget*> BrowserStack::Browsers() const {
  QList<BrowserWidget*> ret;
  for (int i = 0; i < count(); i++) {
    auto browser = qobject_cast<BrowserWidget*>(widget(i));
    if (browser) ret.append(browser);
  }
  return ret;
}

void BrowserStack::SetupActions() {
  connect(ActionManager::Action(ActionManager::FocusAddressBar),
          &QAction::triggered, [=]() {
    auto browser = CurrentBrowser();
    if (browser) browser->FocusUrlEdit();
  });
  connect(ActionManager::Action(ActionManager::FocusBrowser),
          &QAction::triggered, [=]() {
    auto browser = CurrentBrowser();
    if (browser) browser->FocusBrowser();
  });

  // Let's disable/enable a bunch of actions on change
  connect(this, &BrowserStack::CurrentBrowserOrLoadingStateChanged, [=]() {
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
    enabled(ActionManager::ClosePage, current);
  });
  emit CurrentBrowserOrLoadingStateChanged();
}

}  // namespace doogie
