#include <QApplication>

#include "action_manager.h"
#include "cef/cef.h"
#include "main_window.h"
#include "page_index.h"
#include "profile.h"

#ifdef QT_DEBUG
#include "debug_meta_server.h"
#endif

int main(int argc, char* argv[]) {
  doogie::Cef cef(argc, argv);
  if (cef.EarlyExitCode() >= 0) return cef.EarlyExitCode();

  QApplication app(argc, argv);
  QCoreApplication::setOrganizationName("cretz");
  QCoreApplication::setApplicationName("Doogie");
  doogie::Profile::Current()->Init();
  doogie::ActionManager::CreateInstance(&app);

  doogie::PageIndex::Expirer expirer;

  doogie::MainWindow win(cef);
  win.show();
  win.activateWindow();
  win.raise();

#ifdef QT_DEBUG
  doogie::DebugMetaServer meta_server(&win);
#endif

  return app.exec();
}
