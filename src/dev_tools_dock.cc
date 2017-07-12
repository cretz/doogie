#include "dev_tools_dock.h"

DevToolsDock::DevToolsDock(BrowserStack *browser_stack, QWidget *parent)
    : QDockWidget("Dev Tools", parent), browser_stack_(browser_stack) {
  setFeatures(QDockWidget::AllDockWidgetFeatures);

  auto not_there_label = new QLabel("Dev tools not opened for current page");
  not_there_label->setAlignment(Qt::AlignCenter);
  not_there_label->autoFillBackground();

  tools_stack_ = new QStackedWidget();
  tools_stack_->addWidget(not_there_label);
  setWidget(tools_stack_);
}

void DevToolsDock::BrowserChanged(BrowserWidget* browser) {

}

void DevToolsDock::ShowDevTools(BrowserWidget *browser) {

}
