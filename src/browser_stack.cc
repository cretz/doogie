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
  if (resource_load_callback_) {
    widg->SetResourceLoadCallback(resource_load_callback_);
  }
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
  connect(widg, &BrowserWidget::destroyed, [=](QObject*) {
    emit BrowserDestroyed(widg);
  });
  // We load the URL separately so we can have the loading icon and what not
  if (!url.isEmpty()) widg->LoadUrl(url);
  widg->resize(size());
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

void BrowserStack::SetResourceLoadCallback(
    BrowserWidget::ResourceLoadCallback callback) {
  resource_load_callback_ = callback;
  for (auto& browser : Browsers()) {
    browser->SetResourceLoadCallback(resource_load_callback_);
  }
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
    enabled(ActionManager::Fullscreen, current);
    enabled(ActionManager::ZoomIn, current);
    enabled(ActionManager::ZoomOut, current);
    enabled(ActionManager::ResetZoom, current);
    enabled(ActionManager::FindInPage, current);
    enabled(ActionManager::ClosePage, current);
  });
  emit CurrentBrowserOrLoadingStateChanged();
}

}  // namespace doogie
