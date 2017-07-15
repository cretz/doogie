#ifndef DOOGIE_MAINWINDOW_H_
#define DOOGIE_MAINWINDOW_H_

#include <QtWidgets>
#include "cef.h"
#include "cef_widget.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(Cef *cef, QWidget *parent = nullptr);
  ~MainWindow();

 protected:
  void keyPressEvent(QKeyEvent *event);
  void timerEvent(QTimerEvent *event);

 private:
  Cef *cef_;
};

#endif // DOOGIE_MAINWINDOW_H_
