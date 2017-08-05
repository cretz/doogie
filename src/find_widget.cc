#include "find_widget.h"

#include "util.h"

namespace doogie {

FindWidget::FindWidget(QWidget* parent) : QFrame(parent) {
  setFrameShape(QFrame::StyledPanel);
  auto layout = new QHBoxLayout;
  layout->setMargin(2);

  layout->addWidget(new QLabel("  Find:"));

  find_text_ = new QLineEdit;
  setFocusProxy(find_text_);
  find_text_->setStyleSheet(
      "QLineEdit[search_fail=true] { background-color: rgb(239, 91, 79); }");
  layout->addWidget(find_text_, 1);

  auto find_prev = new QPushButton(
        Util::CachedIcon(":/res/images/fontawesome/chevron-up.png"), "");
  find_prev->setToolTip("Find Previous");
  find_prev->setFlat(true);
  layout->addWidget(find_prev);

  auto find_next = new QPushButton(
        Util::CachedIcon(":/res/images/fontawesome/chevron-down.png"), "");
  find_next->setToolTip("Find Next");
  find_next->setFlat(true);
  layout->addWidget(find_next);

  auto match_case = new QCheckBox("Match Case");
  layout->addWidget(match_case);

  find_results_ = new QLabel;
  layout->addWidget(find_results_, 1, Qt::AlignHCenter);

  auto close = new QPushButton(
        Util::CachedIcon(":/res/images/fontawesome/times.png"), "");
  close->setToolTip("Close");
  close->setFlat(true);
  connect(close, &QPushButton::clicked, [=]() {
    hide();
  });
  layout->addWidget(close);

  setLayout(layout);

  connect(find_text_, &QLineEdit::textEdited, [=](const QString&) {
    if (find_text_->text().isEmpty()) UpdateFindResults(0, 0, false);
    emit AttemptFind(find_text_->text(), true, match_case->isChecked(), false);
  });
  connect(find_text_, &QLineEdit::returnPressed, [=]() {
    auto forward = !QApplication::keyboardModifiers().
        testFlag(Qt::ShiftModifier);
    emit AttemptFind(find_text_->text(), forward,
                     match_case->isChecked(), unchanged_means_continue_);
    unchanged_means_continue_ = true;
  });
  connect(find_prev, &QPushButton::clicked, [=]() {
    emit AttemptFind(find_text_->text(), false, match_case->isChecked(),
                     unchanged_means_continue_);
    unchanged_means_continue_ = true;
  });
  connect(find_next, &QPushButton::clicked, [=]() {
    emit AttemptFind(find_text_->text(), true, match_case->isChecked(),
                     unchanged_means_continue_);
    unchanged_means_continue_ = true;
  });
}

void FindWidget::FindResult(int count, int index) {
  UpdateFindResults(count, index, true);
}

void FindWidget::focusInEvent(QFocusEvent* event) {
  find_text_->selectAll();
  QFrame::focusInEvent(event);
}

void FindWidget::hideEvent(QHideEvent* event) {
  emit Hidden();
  QFrame::hideEvent(event);
}

void FindWidget::keyReleaseEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Escape) {
    hide();
  } else {
    QFrame::keyReleaseEvent(event);
  }
}

void FindWidget::showEvent(QShowEvent* event) {
  unchanged_means_continue_ = false;
  find_text_->selectAll();
  UpdateFindResults(0, 0, false);
  QFrame::showEvent(event);
}

void FindWidget::UpdateFindResults(int count, int index, bool real_search) {
  auto search_fail = false;
  if (real_search) {
    search_fail = count == 0;
    if (count == 0) {
      find_results_->setText("No results");
    } else if (count == 1) {
      find_results_->setText("1 of 1 result");
    } else {
      find_results_->setText(QString("%1 of %2 results").
          arg(index).arg(count));
    }
  } else {
    find_results_->setText("");
  }
  if (find_text_->property("search_fail") != search_fail) {
    find_text_->setProperty("search_fail", search_fail);
    find_text_->style()->unpolish(find_text_);
    find_text_->style()->polish(find_text_);
  }
}

}  // namespace doogie
