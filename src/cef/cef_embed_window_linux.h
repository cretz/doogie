#ifndef DOOGIE_CEF_EMBED_WINDOW_LINUX_H_
#define DOOGIE_CEF_EMBED_WINDOW_LINUX_H_

#include <QtWidgets>

#include "cef/cef.h"
#include "cef/cef_base_widget.h"

namespace doogie {

// Window for embedding CEF on Linux.
class CefEmbedWindow : public QWindow {
  Q_OBJECT

 public:
  explicit CefEmbedWindow(QPointer<CefBaseWidget> cef_base_widget,
                          QWindow* parent = nullptr);

 protected:
  void moveEvent(QMoveEvent*) override;
  void resizeEvent(QResizeEvent*) override;

 private:
  QPointer<CefBaseWidget> cef_base_widget_;
};

}  // namespace doogie

#endif  // DOOGIE_CEF_EMBED_WINDOW_LINUX_H_
