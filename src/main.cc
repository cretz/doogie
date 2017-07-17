#include <QApplication>
#include "main_window.h"
#include "cef.h"

#ifdef QT_DEBUG
#include "debug_meta_server.h"
#endif

int main(int argc, char* argv[]) {
  doogie::Cef cef(argc, argv);
  if (cef.EarlyExitCode() >= 0) return cef.EarlyExitCode();

  QApplication app(argc, argv);

  doogie::MainWindow win(&cef);
  win.show();
  win.activateWindow();
  win.raise();

#ifdef QT_DEBUG
  doogie::DebugMetaServer meta_server(&win);
#endif

  return app.exec();
}
