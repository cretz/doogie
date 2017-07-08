#ifndef DOOGIE_CEFEMBEDWINDOW_H_
#define DOOGIE_CEFEMBEDWINDOW_H_

#include <QtWidgets>
#include "cef.h"
#include "cef_widget.h"

class CefEmbedWindow : public QWindow {
  Q_OBJECT
 public:
  CefEmbedWindow(QPointer<CefWidget> cef_widget, QWindow *parent = nullptr);
 protected:
  void moveEvent(QMoveEvent *);
  void resizeEvent(QResizeEvent *);
 private:
  QPointer<CefWidget> cef_widget_;
};

#endif // DOOGIE_CEFEMBEDWINDOW_H_
