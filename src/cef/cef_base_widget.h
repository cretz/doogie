#ifndef DOOGIE_CEF_BASE_WIDGET_H_
#define DOOGIE_CEF_BASE_WIDGET_H_

#include <QtWidgets>

#include "cef/cef.h"
#include "cef/cef_handler.h"

namespace doogie {

class CefBaseWidget : public QWidget {
  Q_OBJECT

 public:
  explicit CefBaseWidget(const Cef& cef, QWidget* parent = nullptr);
  ~CefBaseWidget();

  const CefWindowInfo& WindowInfo() const;
  void ForwardKeyboardEventsFrom(CefRefPtr<CefHandler> handler);

 protected:
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  virtual void UpdateSize();

  const Cef& cef_;
  CefWindowInfo window_info_;

 private:
  void InitWindowInfo();
};

}  // namespace doogie

#endif  // DOOGIE_CEF_BASE_WIDGET_H_
