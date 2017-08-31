#include <QApplication>

#include "action_manager.h"
#include "cef/cef.h"
#include "main_window.h"
#include "updater.h"

#ifdef QT_DEBUG
#include "debug_meta_server.h"
#endif

int main(int argc, char* argv[]) {
  doogie::Cef cef(argc, argv);
  if (cef.EarlyExitCode() >= 0) return cef.EarlyExitCode();

  QApplication app(argc, argv);
  QCoreApplication::setOrganizationName("cretz");
  QCoreApplication::setApplicationName("Doogie");
  doogie::ActionManager::CreateInstance(&app);

  // Creating this is enough to start it
  doogie::Updater updater(cef);

  doogie::MainWindow win(cef);
  win.show();
  win.activateWindow();
  win.raise();

#ifdef QT_DEBUG
  doogie::DebugMetaServer meta_server(&win);
#endif

  return app.exec();
}
