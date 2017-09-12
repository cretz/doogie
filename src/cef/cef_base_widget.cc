#include "cef/cef_base_widget.h"

namespace doogie {

CefBaseWidget::CefBaseWidget(const Cef& cef, QWidget*  parent)
    : QWidget(parent), cef_(cef) {
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
  // For now, we forward everything but the tab key sans mods
  if (event.character != 9) return true;
  bool has_mods = event.modifiers & EVENTFLAG_SHIFT_DOWN ||
        event.modifiers & EVENTFLAG_CONTROL_DOWN ||
        event.modifiers & EVENTFLAG_ALT_DOWN ||
        event.modifiers & EVENTFLAG_COMMAND_DOWN;
  return has_mods;
}

}  // namespace doogie
