#ifndef DOOGIE_CEF_BASE_WIDGET_H_
#define DOOGIE_CEF_BASE_WIDGET_H_

#include <QtWidgets>
#include "cef.h"
#include "cef_handler.h"

namespace doogie {

class CefBaseWidget : public QWidget {
  Q_OBJECT

 public:
  CefBaseWidget(Cef* cef, QWidget* parent = nullptr);
  ~CefBaseWidget();

  const CefWindowInfo& WindowInfo();
  void ForwardKeyboardEventsFrom(CefRefPtr<CefHandler> handler);

 protected:
  virtual void moveEvent(QMoveEvent* event) override;
  virtual void resizeEvent(QResizeEvent* event) override;
  virtual void UpdateSize();

  Cef* cef_;
  CefWindowInfo window_info_;

 private:
  void InitWindowInfo();
};

}  // namespace doogie

#endif  // DOOGIE_CEF_BASE_WIDGET_H_
