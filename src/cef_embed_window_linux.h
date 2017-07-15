#ifndef DOOGIE_CEF_EMBED_WINDOW_LINUX_H_
#define DOOGIE_CEF_EMBED_WINDOW_LINUX_H_

#include <QtWidgets>
#include "cef.h"
#include "cef_widget.h"

namespace doogie {

class CefEmbedWindow : public QWindow {
  Q_OBJECT

 public:
  CefEmbedWindow(QPointer<CefWidget> cef_widget, QWindow* parent = nullptr);

 protected:
  void moveEvent(QMoveEvent*) override;
  void resizeEvent(QResizeEvent*) override;

 private:
  QPointer<CefWidget> cef_widget_;
};

}  // namespace doogie

#endif  // DOOGIE_CEF_EMBED_WINDOW_LINUX_H_
