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
  virtual ~CefBaseWidget();

  const CefWindowInfo& WindowInfo() const;
  void ForwardKeyboardEventsFrom(CefRefPtr<CefHandler> handler);

  QWidget* ViewWidget() {
    return override_widget_ ? override_widget_ : this;
  }

 protected:
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
#if defined(__GNUC__)
  friend class CefEmbedWindow;
#endif
  virtual void UpdateSize();

  const Cef& cef_;
  CefWindowInfo window_info_;
  QPointer<QWidget> override_widget_;

 private:
  void InitWindowInfo();
};

}  // namespace doogie

#endif  // DOOGIE_CEF_BASE_WIDGET_H_
