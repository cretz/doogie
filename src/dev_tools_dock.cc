#include "dev_tools_dock.h"

namespace doogie {

DevToolsDock::DevToolsDock(const Cef& cef,
                           BrowserStack* browser_stack,
                           QWidget* parent)
    : QDockWidget("Dev Tools", parent),
      cef_(cef),
      browser_stack_(browser_stack) {
  setFeatures(QDockWidget::AllDockWidgetFeatures);

  connect(browser_stack_, &BrowserStack::BrowserChanged,
          this, &DevToolsDock::BrowserChanged);

  auto not_there_label = new QLabel("No browser open");
  not_there_label->setAlignment(Qt::AlignCenter);
  not_there_label->autoFillBackground();

  auto open_dev_tools_btn = new QPushButton("Click to open dev tools");
  connect(open_dev_tools_btn, &QPushButton::clicked, [=](bool) {
    ShowDevTools(browser_stack_->CurrentBrowser(), QPoint());
  });
  auto open_layout = new QGridLayout();
  open_layout->setColumnStretch(0, 1);
  open_layout->setRowStretch(0, 1);
  open_layout->addWidget(open_dev_tools_btn, 1, 1);
  open_layout->setColumnStretch(2, 1);
  open_layout->setRowStretch(2, 1);
  auto open_widg = new QWidget();
  open_widg->setLayout(open_layout);

  tools_stack_ = new QStackedWidget();
  tools_stack_->addWidget(not_there_label);
  tools_stack_->addWidget(open_widg);
  setWidget(tools_stack_);

  // Go ahead and invoke as though the browser changed
  BrowserChanged(browser_stack->CurrentBrowser());
}

bool DevToolsDock::DevToolsShowing() const {
  return tools_stack_->currentIndex() > 1;
}

void DevToolsDock::BrowserChanged(BrowserWidget* browser) {
  if (!browser) {
    tools_stack_->setCurrentIndex(0);
  } else {
    auto tools_widg = tools_widgets_[browser];
    if (tools_widg) {
      tools_stack_->setCurrentWidget(tools_widg->ViewWidget());
    } else {
      tools_stack_->setCurrentIndex(1);
    }
  }
}

void DevToolsDock::ShowDevTools(BrowserWidget* browser,
                                const QPoint& inspect_at) {
  // This can be called repeatedly for the same browser
  auto base_widg = tools_widgets_[browser];
  if (!base_widg) {
    base_widg = new CefBaseWidget(cef_, tools_stack_);
    tools_stack_->addWidget(base_widg->ViewWidget());
    tools_widgets_[browser] = base_widg;
    // We make "this" the context of the connections so we can
    //  disconnect later
    // Need to show that close button
    connect(browser, &BrowserWidget::DevToolsLoadComplete, this, [=]() {
      // TODO(cretz): put a unit test around this highly-volatile code
      QString js =
          "Components.dockController._closeButton.setVisible(true);\n"
          "Components.dockController._closeButton.addEventListener(\n"
          "  UI.ToolbarButton.Events.Click,\n"
          "  () => window.close()\n"
          ");\n";
      browser->ExecDevToolsJs(js);
    });
    // On close, remove it
    connect(browser, &BrowserWidget::DevToolsClosed,
            this, [=]() { DevToolsClosed(browser); });
    // On destroy, remove it
    connect(browser, &BrowserWidget::destroyed,
            this, [=]() { DevToolsClosed(browser); });
  }
  browser->ShowDevTools(base_widg, inspect_at);
  tools_stack_->setCurrentWidget(base_widg->ViewWidget());
}

void DevToolsDock::CloseDevTools(BrowserWidget* browser) {
  // Call DevToolsClosed first so we can get it out of the map
  DevToolsClosed(browser);
  browser->CloseDevTools();
}

void DevToolsDock::closeEvent(QCloseEvent* event) {
  QDockWidget::closeEvent(event);
  auto keys = tools_widgets_.keys();
  for (int i = 0; i < keys.size(); i++) {
    CloseDevTools(keys[0]);
  }
}

void DevToolsDock::DevToolsClosed(BrowserWidget* browser) {
  auto base_widg = tools_widgets_[browser];
  if (base_widg) {
    tools_widgets_.remove(browser);
    disconnect(browser, &BrowserWidget::DevToolsLoadComplete, this, nullptr);
    disconnect(browser, &BrowserWidget::DevToolsClosed, this, nullptr);
    disconnect(browser, &BrowserWidget::destroyed, this, nullptr);
    tools_stack_->removeWidget(base_widg->ViewWidget());
    delete base_widg;
  }
}

}  // namespace doogie
