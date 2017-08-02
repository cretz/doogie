#include "settings_widget.h"

namespace doogie {

SettingsWidget::SettingsWidget(QWidget* parent) : QScrollArea(parent) {
  layout_ = new QGridLayout;
  layout_->setVerticalSpacing(3);
  auto widg = new QWidget;
  widg->setLayout(layout_);
  setFrameShape(QFrame::NoFrame);
  setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
  setWidget(widg);
  setWidgetResizable(true);
}

void SettingsWidget::AddSetting(const QString& name,
                                const QString& desc,
                                QWidget* widg) {
  auto name_label = new QLabel(name);
  name_label->setStyleSheet("font-weight: bold;");
  auto row = layout_->rowCount();
  layout_->addWidget(name_label, row, 0);
  auto desc_label = new QLabel(desc);
  desc_label->setWordWrap(true);
  layout_->addWidget(widg, row, 1);
  layout_->addWidget(desc_label, row + 1, 0, 1, 2);
}

QComboBox* SettingsWidget::AddComboBoxSetting(const QString& name,
                                              const QString& desc,
                                              QStringList items,
                                              int selected) {
  auto widg = new QComboBox;
  widg->addItems(items);
  widg->setCurrentIndex(selected);
  AddSetting(name, desc, widg);
  return widg;
}

QComboBox* SettingsWidget::AddYesNoSetting(const QString& name,
                                           const QString& desc,
                                           bool curr,
                                           bool default_val) {
  QStringList items({
    QString("Yes") + (default_val ? " (default)" : ""),
    QString("No") + (!default_val ? " (default)" : "")
  });
  return AddComboBoxSetting(name, desc, items, curr ? 0 : 1);
}

void SettingsWidget::AddSettingBreak() {
  auto frame = new QFrame;
  frame->setFrameShape(QFrame::HLine);
  layout_->addWidget(frame, layout_->rowCount(), 0, 1, 2);
}

}  // namespace doogie
