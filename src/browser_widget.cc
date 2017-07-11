#include "browser_widget.h"

BrowserWidget::BrowserWidget(Cef *cef,
                             const QString &url,
                             QWidget *parent)
    : QWidget(parent), cef_(cef) {
  cef_widg_ = new CefWidget(cef, url, this);
  connect(cef_widg_, &CefWidget::TitleChanged, [this](const QString &title) {
    current_title_ = title;
    emit TitleChanged();
  });
  connect(cef_widg_, &CefWidget::FaviconChanged, [this](const QIcon &icon) {
    current_favicon_ = icon;
    emit FaviconChanged();
  });
  connect(cef_widg_, &CefWidget::LoadStateChanged,
          [this](bool is_loading, bool can_go_back, bool can_go_forward) {
    loading_ = is_loading;
    can_go_back_ = can_go_back;
    can_go_forward_ = can_go_forward;
    emit LoadingStateChanged();
  });
  connect(cef_widg_, &CefWidget::PageOpen,
          [this](CefRequestHandler::WindowOpenDisposition type,
                 const QString &url,
                 bool user_gesture) {
    emit PageOpen((WindowOpenType) type, url, user_gesture);
  });

  url_edit_ = new QLineEdit(this);
  connect(url_edit_, &QLineEdit::returnPressed, [this]() {
    cef_widg_->LoadUrl(url_edit_->text());
    cef_widg_->setFocus();
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
  url_edit_->activateWindow();
}

void BrowserWidget::FocusBrowser() {
  cef_widg_->setFocus();
}

QIcon BrowserWidget::CurrentFavicon() {
  return current_favicon_;
}

QString BrowserWidget::CurrentTitle() {
  return current_title_;
}

bool BrowserWidget::Loading() {
  return loading_;
}

bool BrowserWidget::CanGoBack() {
  return can_go_back_;
}

bool BrowserWidget::CanGoForward() {
  return can_go_forward_;
}

void BrowserWidget::moveEvent(QMoveEvent *) {
  this->UpdateStatusBarLocation();
}

void BrowserWidget::resizeEvent(QResizeEvent *) {
  this->UpdateStatusBarLocation();
}

void BrowserWidget::UpdateStatusBarLocation() {
  status_bar_->move(0, this->height() - status_bar_->height());
}
