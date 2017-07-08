#include "browser_widget.h"

BrowserWidget::BrowserWidget(Cef *cef,
                             const QString &url,
                             QWidget *parent)
    : QWidget(parent), cef_(cef) {
  cef_widg_ = new CefWidget(cef, url, this);
  connect(cef_widg_, &CefWidget::TitleChanged,
          this, &BrowserWidget::TitleChanged);
  connect(cef_widg_, &CefWidget::FaviconChanged,
          this, &BrowserWidget::FaviconChanged);
  connect(cef_widg_, &CefWidget::TabOpen,
          [this](CefRequestHandler::WindowOpenDisposition type,
                 const QString &url,
                 bool user_gesture) {
    emit TabOpen((WindowOpenType) type, url, user_gesture);
  });

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

  status_bar_ = new QStatusBar(this);
  status_bar_->setSizeGripEnabled(false);
  status_bar_->hide();
  // TODO: intelligently set the width based on message size,
  //  placement of scrollbars, mouse position, and window size
  // TODO: handle overflow w/ ellipses
  status_bar_->resize(300, status_bar_->height());
  this->UpdateStatusBarLocation();
  connect(cef_widg_, &CefWidget::StatusChanged, [this](const QString &status) {
    if (status.isEmpty()) {
      status_bar_->hide();
      status_bar_->clearMessage();
    } else {
      status_bar_->showMessage(status);
      status_bar_->show();
    }
  });
}

void BrowserWidget::FocusUrlEdit() {
  url_edit_->setFocus();
}

void BrowserWidget::FocusBrowser() {
  cef_widg_->setFocus();
}

void BrowserWidget::moveEvent(QMoveEvent *event) {
  this->UpdateStatusBarLocation();
}

void BrowserWidget::resizeEvent(QResizeEvent *event) {
  this->UpdateStatusBarLocation();
}

void BrowserWidget::UpdateStatusBarLocation() {
  status_bar_->move(0, this->height() - status_bar_->height());
}
