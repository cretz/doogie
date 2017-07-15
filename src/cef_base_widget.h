#ifndef DOOGIE_CEFBASEWIDGET_H_
#define DOOGIE_CEFBASEWIDGET_H_

#include <QtWidgets>
#include "cef.h"
#include "cef_handler.h"

class CefBaseWidget : public QWidget {
  Q_OBJECT
 public:
  CefBaseWidget(Cef *cef, QWidget *parent = nullptr);
  ~CefBaseWidget();

  const CefWindowInfo& WindowInfo();
  void ForwardKeyboardEventsFrom(CefRefPtr<CefHandler> handler);

 protected:
  Cef* cef_;
  CefWindowInfo window_info_;
  virtual void moveEvent(QMoveEvent *event) override;
  virtual void resizeEvent(QResizeEvent *event) override;
  virtual void UpdateSize();

 private:
  void InitWindowInfo();
};

#endif // DOOGIE_CEFBASEWIDGET_H_
