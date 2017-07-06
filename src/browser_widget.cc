#include "browser_widget.h"

BrowserWidget::BrowserWidget(Cef *cef, QWidget *parent) : QWidget(parent), cef_(cef) {
  cef_widg_ = new CefWidget(cef, this);

  url_edit_ = new QLineEdit(this);
  connect(url_edit_, &QLineEdit::returnPressed, [this]() {
    cef_widg_->LoadUrl(url_edit_->text());
  });
  auto layout = new QGridLayout;
  layout->addWidget(url_edit_, 0, 0);
  layout->addWidget(cef_widg_, 1, 0);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->setRowStretch(0, 0);
  layout->setRowStretch(1, 1);
  setLayout(layout);

  auto override_widg = cef_widg_->OverrideWidget();
  if (override_widg) {
    layout->addWidget(override_widg, 1, 0);
  }

  connect(cef_widg_, &CefWidget::UrlChanged, [this](const QString &url) {
    url_edit_->setText(url);
  });
}
