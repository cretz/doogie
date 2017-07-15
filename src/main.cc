#include <QApplication>
#include "main_window.h"
#include "cef.h"

int main(int argc, char* argv[]) {
  doogie::Cef cef(argc, argv);
  if (cef.EarlyExitCode() >= 0) return cef.EarlyExitCode();

  QApplication app(argc, argv);

  doogie::MainWindow win(&cef);
  win.show();
  win.activateWindow();
  win.raise();

  return app.exec();
}
