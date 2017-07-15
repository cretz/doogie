#include "browser_widget.h"
#include "util.h"

namespace doogie {

BrowserWidget::BrowserWidget(Cef* cef,
                             const QString& url,
                             QWidget* parent)
    : QWidget(parent), cef_(cef) {

  nav_menu_ = new QMenu(this);
  // When this menu is about to be opened we have to populate the items
  connect(nav_menu_, &QMenu::aboutToShow,
          this, &BrowserWidget::RebuildNavMenu);

  back_button_ = new QToolButton();
  back_button_->setText("Back");
  back_button_->setMenu(nav_menu_);
  back_button_->setIcon(Util::CachedIconLighterDisabled(
      ":/res/images/fontawesome/arrow-left.png"));
  back_button_->setAutoRaise(true);
  back_button_->setDisabled(true);
  connect(back_button_, &QToolButton::clicked, [this](bool) { Back(); });

  forward_button_ = new QToolButton();
  forward_button_->setText("Forward");
  forward_button_->setMenu(nav_menu_);
  forward_button_->setIcon(Util::CachedIconLighterDisabled(
      ":/res/images/fontawesome/arrow-right.png"));
  forward_button_->setAutoRaise(true);
  forward_button_->setDisabled(true);
  connect(forward_button_, &QToolButton::clicked, [this](bool) { Forward(); });

  url_edit_ = new QLineEdit(this);
  connect(url_edit_, &QLineEdit::returnPressed, [this]() {
    cef_widg_->LoadUrl(url_edit_->text());
    cef_widg_->setFocus();
  });

  refresh_button_ = new QToolButton();
  refresh_button_->setText("Refresh");
  refresh_button_->setIcon(Util::CachedIconLighterDisabled(
      ":/res/images/fontawesome/repeat.png"));
  refresh_button_->setAutoRaise(true);
  refresh_button_->setDisabled(true);
  connect(refresh_button_, &QToolButton::clicked, [this](bool) { Refresh(); });

  stop_button_ = new QToolButton();
  stop_button_->setText("Refresh");
  stop_button_->setIcon(
        Util::CachedIcon(":/res/images/fontawesome/times-circle.png"));
  stop_button_->setAutoRaise(true);
  stop_button_->setVisible(false);
  connect(stop_button_, &QToolButton::clicked, [this](bool) { Stop(); });

  auto top_layout = new QHBoxLayout();
  top_layout->setMargin(0);
  top_layout->addWidget(back_button_);
  top_layout->addWidget(forward_button_);
  top_layout->addWidget(url_edit_, 1);
  top_layout->addWidget(stop_button_);
  top_layout->addWidget(refresh_button_);
  auto top_widg = new QWidget;
  top_widg->setLayout(top_layout);

  cef_widg_ = new CefWidget(cef, url, this);
  connect(cef_widg_, &CefWidget::TitleChanged, [this](const QString& title) {
    current_title_ = title;
    emit TitleChanged();
  });
  connect(cef_widg_, &CefWidget::FaviconChanged, [this](const QIcon& icon) {
    current_favicon_ = icon;
    emit FaviconChanged();
  });
  connect(cef_widg_, &CefWidget::LoadStateChanged,
          [this](bool is_loading, bool can_go_back, bool can_go_forward) {
    loading_ = is_loading;
    can_go_back_ = can_go_back;
    can_go_forward_ = can_go_forward;
    refresh_button_->setEnabled(true);
    refresh_button_->setVisible(!is_loading);
    stop_button_->setVisible(is_loading);
    back_button_->setDisabled(!can_go_back);
    forward_button_->setDisabled(!can_go_forward);
    emit LoadingStateChanged();
  });
  connect(cef_widg_, &CefWidget::PageOpen,
          [this](CefRequestHandler::WindowOpenDisposition type,
                 const QString& url,
                 bool user_gesture) {
    emit PageOpen(static_cast<WindowOpenType>(type), url, user_gesture);
  });
  connect(cef_widg_, &CefWidget::DevToolsLoadComplete,
          this, &BrowserWidget::DevToolsLoadComplete);
  connect(cef_widg_, &CefWidget::DevToolsClosed,
          this, &BrowserWidget::DevToolsClosed);

  auto layout = new QGridLayout;
  layout->addWidget(top_widg, 0, 0);
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

  connect(cef_widg_, &CefWidget::UrlChanged, [this](const QString& url) {
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
  connect(cef_widg_, &CefWidget::StatusChanged, [this](const QString& status) {
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
  // We choose to select the entire text here to make nav easier
  url_edit_->selectAll();
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

void BrowserWidget::Refresh() {
  auto ignore_cache = QApplication::keyboardModifiers().
      testFlag(Qt::ControlModifier);
  cef_widg_->Refresh(ignore_cache);
}

void BrowserWidget::Stop() {
  cef_widg_->Stop();
}

void BrowserWidget::Back() {
  cef_widg_->Go(-1);
}

void BrowserWidget::Forward() {
  cef_widg_->Go(1);
}

void BrowserWidget::Print() {
  cef_widg_->Print();
}

void BrowserWidget::ShowDevTools(CefBaseWidget* widg) {
  cef_widg_->ShowDevTools(widg);
}

void BrowserWidget::ExecDevToolsJs(const QString& js) {
  cef_widg_->ExecDevToolsJs(js);
}

void BrowserWidget::CloseDevTools() {
  cef_widg_->CloseDevTools();
}

double BrowserWidget::GetZoomLevel() {
  return cef_widg_->GetZoomLevel();
}

void BrowserWidget::SetZoomLevel(double level) {
  cef_widg_->SetZoomLevel(level);
}

void BrowserWidget::moveEvent(QMoveEvent*) {
  this->UpdateStatusBarLocation();
}

void BrowserWidget::resizeEvent(QResizeEvent*) {
  this->UpdateStatusBarLocation();
}

void BrowserWidget::UpdateStatusBarLocation() {
  status_bar_->move(0, this->height() - status_bar_->height());
}

void BrowserWidget::RebuildNavMenu() {
  nav_menu_->clear();
  auto entries = cef_widg_->NavEntries();
  // We need to find the "current" index so we can do neg/pos index from that
  int current_item_index = 0;
  for (int i = 0; i < entries.size(); i++) {
    if (entries[i].current) {
      current_item_index = i;
      break;
    }
  }
  // Now that we have the current, we know the index to go back or forward
  // We have to go backwards...
  for (int i = static_cast<int>(entries.size()) - 1; i >= 0; i--) {
    auto entry = entries[i];
    auto action = nav_menu_->addAction(entry.title);
    auto nav_index = i - current_item_index;
    if (nav_index != 0) {
      connect(action, &QAction::triggered, [this, nav_index]() {
        cef_widg_->Go(nav_index);
      });
    } else {
      // Bold the current
      auto new_font = action->font();
      new_font.setBold(true);
      action->setFont(new_font);
    }
  }
}

}  // namespace doogie
