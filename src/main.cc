#include <QApplication>

#include "action_manager.h"
#include "cef/cef.h"
#include "main_window.h"
#include "updater.h"

#ifdef QT_DEBUG
#include "debug_meta_server.h"
#endif

int main(int argc, char* argv[]) {
  // We have to set the library path to the exe dir, but we can't use something
  //  like QCoreApplication::applicationDirPath because it requires that we
  //  have an application created already which we don't (and don't want to
  //  because this code is also run for each child process)
  QStringList lib_paths({});
#ifdef Q_OS_UNIX
  QFileInfo pfi(QString::fromLatin1("/proc/%1/exe").arg(getpid()));
  if (pfi.exists() && pfi.isSymLink()) {
    auto exe_dir = QFileInfo(pfi.canonicalFilePath()).path();
    lib_paths << exe_dir;
  }
#endif
  QCoreApplication::setLibraryPaths(lib_paths);

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
