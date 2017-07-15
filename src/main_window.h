#ifndef DOOGIE_MAIN_WINDOW_H_
#define DOOGIE_MAIN_WINDOW_H_

#include <QtWidgets>
#include "cef.h"
#include "cef_widget.h"

namespace doogie {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(Cef* cef, QWidget* parent = nullptr);
  ~MainWindow();

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  void timerEvent(QTimerEvent* event) override;

 private:
  Cef* cef_;
};

}  // namespace doogie

#endif  // DOOGIE_MAIN_WINDOW_H_
