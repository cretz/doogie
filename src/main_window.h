#ifndef DOOGIE_MAINWINDOW_H_
#define DOOGIE_MAINWINDOW_H_

#include <QtWidgets>
#include "cef.h"
#include "cef_widget.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(Cef *cef, QWidget *parent = 0);
  ~MainWindow();

 protected:
  void timerEvent(QTimerEvent *event);

 private:
  Cef *cef_;
  CefWidget *cef_widg_;
  QLineEdit *url_line_edit_;
  QGridLayout *layout;

 private slots:
  void UrlEntered();
};

#endif // DOOGIE_MAINWINDOW_H_
