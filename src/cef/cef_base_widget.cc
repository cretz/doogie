#include "cef/cef_base_widget.h"

namespace doogie {

CefBaseWidget::CefBaseWidget(const Cef& cef,
                             QWidget* parent,
                             const QSize& initial_size)
    : QWidget(parent), cef_(cef) {
  if (!initial_size.isEmpty()) resize(initial_size);
  InitWindowInfo();
}

CefBaseWidget::~CefBaseWidget() {
}

const CefWindowInfo& CefBaseWidget::WindowInfo() const {
  return window_info_;
}

void CefBaseWidget::moveEvent(QMoveEvent* event) {
  UpdateSize();
  QWidget::moveEvent(event);
}

void CefBaseWidget::resizeEvent(QResizeEvent* event) {
  UpdateSize();
  QWidget::resizeEvent(event);
}

bool CefBaseWidget::IsForwardableKeyEvent(const CefKeyEvent& event) const {
  // For now, we forward everything but:
  //  * tab key (unless Ctrl is pressed)
  //  * backspace while in edit field
  // Tab check
  if (event.character == 9) {
    bool ctrl_down = event.modifiers & EVENTFLAG_CONTROL_DOWN ||
        event.modifiers & EVENTFLAG_COMMAND_DOWN;
    return ctrl_down;
  }
  // Backspace check
  if (event.character == 8) {
    return !event.focus_on_editable_field;
  }
  return true;
}

}  // namespace doogie
