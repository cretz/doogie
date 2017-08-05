#ifndef DOOGIE_CEF_EMBED_WINDOW_LINUX_H_
#define DOOGIE_CEF_EMBED_WINDOW_LINUX_H_

#include <QtWidgets>

#include "cef/cef.h"
#include "cef/cef_widget.h"

namespace doogie {

class CefEmbedWindow : public QWindow {
  Q_OBJECT

 public:
  explicit CefEmbedWindow(QPointer<CefWidget> cef_widget,
                          QWindow* parent = nullptr);

 protected:
  void moveEvent(QMoveEvent*) override;
  void resizeEvent(QResizeEvent*) override;

 private:
  QPointer<CefWidget> cef_widget_;
};

}  // namespace doogie

#endif  // DOOGIE_CEF_EMBED_WINDOW_LINUX_H_
