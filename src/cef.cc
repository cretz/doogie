#include "cef.h"
#include <QtGlobal>
#include "profile.h"

namespace doogie {

Cef::Cef(int argc, char* argv[]) {
  CefEnableHighDPISupport();

  CefMainArgs main_args(this->MainArgs(argc, argv));
  app_handler_ = CefRefPtr<CefAppHandler>(new CefAppHandler);

  early_exit_code_ = CefExecuteProcess(main_args, app_handler_, nullptr);
  if (early_exit_code_ >= 0) {
    // Means it was a child process and it blocked until complete, exit
    return;
  }

  // Init the profile, which can fail early
  if (!Profile::LoadProfileFromCommandLine(argc, argv)) {
    qCritical() << "Unable to create or load profile";
    early_exit_code_ = 2;
    return;
  }

  auto settings = Profile::Current()->CreateCefSettings();
  if (!CefInitialize(main_args, settings, app_handler_, nullptr)) {
    throw std::runtime_error("Unable to initialize CEF");
  }
}

Cef::~Cef() {
  if (early_exit_code_ < 0) CefShutdown();
}

int Cef::EarlyExitCode() {
  return early_exit_code_;
}

void Cef::Tick() {
  CefDoMessageLoopWork();
}

CefRefPtr<CefAppHandler> Cef::AppHandler() {
  return app_handler_;
}

}  // namespace doogie
