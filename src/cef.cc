#include "cef.h"

namespace doogie {

Cef::Cef(int argc, char* argv[]) {
  CefEnableHighDPISupport();

  CefMainArgs main_args(this->MainArgs(argc, argv));

  early_exit_code_ = CefExecuteProcess(main_args, nullptr, nullptr);
  if (early_exit_code_ < 0) {
    // Means it is not a child process, so do other init
    CefSettings settings;
    settings.no_sandbox = true;
    if (!CefInitialize(main_args, settings, nullptr, nullptr)) {
      throw std::runtime_error("Unable to initialize CEF");
    }
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

}  // namespace doogie
