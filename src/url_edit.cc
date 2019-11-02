#include "url_edit.h"

#include "cef/cef.h"
#include "page_index.h"

namespace doogie {

UrlEdit::UrlEdit(const Cef& cef, QWidget* parent) :
    QLineEdit(parent), cef_(cef) {
  setPlaceholderText("URL");
  connect(this, &UrlEdit::returnPressed, [=]() {
    // Change the text then say it has been entered
    if (autocomplete_->currentRow() >= 0) {
      setText(autocomplete_->currentItem()->data(
                kRoleAutocompleteUrl).toString());
    }
    if (!text().isEmpty()) UrlEntered();
  });

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
    autocomplete_->setCurrentItem(item);
    emit returnPressed();
  });
  connect(autocomplete_, &QListWidget::currentRowChanged, [=](int row) {
    if (row >= 0) {
      setText(autocomplete_->item(row)->data(kRoleAutocompleteUrl).toString());
    }
  });
  connect(this, &UrlEdit::textEdited, this, &UrlEdit::HandleTextEdit);
}

void UrlEdit::focusInEvent(QFocusEvent* event) {
  QLineEdit::focusInEvent(event);
}

void UrlEdit::focusOutEvent(QFocusEvent* event) {
  autocomplete_->hide();
  QLineEdit::focusOutEvent(event);
}

void UrlEdit::keyPressEvent(QKeyEvent* event) {
  // All my stuff only applies when there are no modifiers
  if (event->modifiers() == Qt::NoModifier) {
    // Hide the auto complete window if we escaped
    if (event->key() == Qt::Key_Escape && autocomplete_->isVisible()) {
      autocomplete_->hide();
      return;
    }
    // Move around the autocomplete window
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
      if (row == -1) {
        setText(typed_before_moved_);
      }
      autocomplete_->setCurrentRow(row);
      return;
    }
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

  auto looks_like_search = text.startsWith("!") || text.contains(" !");
  auto search_item = new QListWidgetItem("Search DuckDuckGo for: " + text);
  search_item->setData(kRoleAutocompleteUrl,
                       QString("https://duckduckgo.com/?q=") +
                         QUrl::toPercentEncoding(text));
  if (looks_like_search) {
    autocomplete_->addItem(search_item);
    incr_height();
  }

  // If it's a valid UR (but doens't look like search), first result is "visit"
  auto looks_like_url = (text.contains(":/") || text.contains(".")) &&
      cef_.IsValidUrl(text);
  auto visit_item = new QListWidgetItem("Visit: " + text);
  visit_item->setData(kRoleAutocompleteUrl, text);
  if (looks_like_url) {
    autocomplete_->addItem(visit_item);
    incr_height();
  }

  for (const auto& suggest : PageIndex::AutocompleteSuggest(text, 10)) {
    auto item = new QListWidgetItem(
          suggest.favicon, suggest.title + " - " + suggest.url);
    item->setData(kRoleAutocompleteUrl, suggest.url);
    autocomplete_->addItem(item);
    incr_height();
  }

  if (!looks_like_search) {
    autocomplete_->addItem(search_item);
    incr_height();
  }
  if (!looks_like_url) {
    autocomplete_->addItem(visit_item);
    incr_height();
  }

  // Set the first one as selected, but block signal so the URL edit
  //  doesn't change
  autocomplete_->blockSignals(true);
  autocomplete_->setCurrentRow(0);
  autocomplete_->blockSignals(false);

  autocomplete_->move(mapToGlobal(rect().bottomLeft()));
  autocomplete_->setFixedWidth(width());
  autocomplete_->setFixedHeight(height);
  autocomplete_->show();
}

}  // namespace doogie
