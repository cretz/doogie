#include "url_edit.h"

#include "page_index.h"

namespace doogie {

UrlEdit::UrlEdit(QWidget* parent) : QLineEdit(parent) {
  setPlaceholderText("URL");

  autocomplete_ = new QListWidget(this);
  autocomplete_->setWindowFlags(
        Qt::Window | Qt::WindowStaysOnTopHint |
        Qt::FramelessWindowHint | Qt::CustomizeWindowHint |
        Qt::WindowDoesNotAcceptFocus);
  autocomplete_->setAttribute(Qt::WA_ShowWithoutActivating);
  autocomplete_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  autocomplete_->setCursor(Qt::PointingHandCursor);
  autocomplete_->hide();
  connect(autocomplete_, &QListWidget::itemClicked,
          [=](QListWidgetItem* item) {
    setText(item->data(Qt::UserRole + 1).toString());
    emit returnPressed();
  });
  connect(autocomplete_, &QListWidget::currentRowChanged, [=](int row) {
    if (row == -1) {
      setText(typed_before_moved_);
    } else {
      setText(autocomplete_->item(row)->data(Qt::UserRole + 1).toString());
    }
  });
  connect(this, &UrlEdit::textEdited, this, &UrlEdit::HandleTextEdit);
}

void UrlEdit::focusOutEvent(QFocusEvent* event) {
  autocomplete_->hide();
  QLineEdit::focusOutEvent(event);
}

void UrlEdit::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Escape && autocomplete_->isVisible()) {
    setText(typed_before_moved_);
    autocomplete_->hide();
    return;
  }
  if (event->key() == Qt::Key_Up || event->key() == Qt::Key_PageUp ||
      event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown) {
    if (!autocomplete_->isVisible()) {
      HandleTextEdit(text());
      return;
    }

    // A result of -1 means we will remove all selection/current
    auto row = autocomplete_->currentRow();
    if (event->key() == Qt::Key_Up) {
      if (row == -1) {
        row = autocomplete_->count() - 1;
      } else if (row == 0) {
        row = -1;
      } else {
        row--;
      }
    } else if (event->key() == Qt::Key_PageUp) {
      if (row == -1) {
        row = autocomplete_->count() - 1;
      } else if (row == 0) {
        row = -1;
      } else if (row < 5) {
        row = 0;
      } else {
        row -= 5;
      }
    } else if (event->key() == Qt::Key_Down) {
      if (row == autocomplete_->count() - 1) {
        row = -1;
      } else {
        row++;
      }
    } else if (event->key() == Qt::Key_PageDown) {
      if (row == -1) {
        row = 0;
      } else if (row == autocomplete_->count() - 1) {
        row = -1;
      } else if (row > autocomplete_->count() - 6) {
        row = autocomplete_->count() - 1;
      } else {
        row += 5;
      }
    }
    autocomplete_->setCurrentRow(row);
    return;
  }
  QLineEdit::keyPressEvent(event);
  typed_before_moved_ = text();
}

void UrlEdit::HandleTextEdit(const QString& text) {
  if (text.isEmpty()) {
    autocomplete_->hide();
    return;
  }

  autocomplete_->clear();
  auto height = 20;
  auto incr_height = [=, &height]() {
    height += autocomplete_->sizeHintForRow(autocomplete_->count() - 1);
  };

  auto starts_with_search = text.startsWith("!");
  auto search_item = new QListWidgetItem("Search DuckDuckGo for: " + text);
  search_item->setData(Qt::UserRole + 1,
                       QString("https://duckduckgo.com/?q=") +
                         QUrl::toPercentEncoding(text));
  if (starts_with_search) {
    autocomplete_->addItem(search_item);
    incr_height();
  }

  for (const auto& suggest : PageIndex::AutocompleteSuggest(text, 10)) {
    auto item = new QListWidgetItem(
          suggest.favicon, suggest.title + " - " + suggest.url);
    item->setData(Qt::UserRole + 1, suggest.url);
    autocomplete_->addItem(item);
    incr_height();
  }

  if (!starts_with_search) {
    autocomplete_->addItem(search_item);
    incr_height();
  }

  autocomplete_->move(mapToGlobal(rect().bottomLeft()));
  autocomplete_->setFixedWidth(width());
  autocomplete_->setFixedHeight(height);
  autocomplete_->show();
}

}  // namespace doogie
