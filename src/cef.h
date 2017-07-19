#ifndef DOOGIE_CEF_H_
#define DOOGIE_CEF_H_

#include "cef_base.h"
#include "cef_app_handler.h"

namespace doogie {

class Cef {
 public:
  Cef(int argc, char* argv[]);
  ~Cef();
  // If >= 0, this is a child and not the UI window
  int EarlyExitCode();
  void Tick();

  CefRefPtr<CefAppHandler> AppHandler();

 private:
  cef_main_args_t MainArgs(int argc, char* argv[]);

  int early_exit_code_;
  CefRefPtr<CefAppHandler> app_handler_;
};

}  // namespace doogie

#endif  // DOOGIE_CEF_H_
