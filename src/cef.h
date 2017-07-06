#ifndef DOOGIE_CEF_H_
#define DOOGIE_CEF_H_

#include "include/cef_app.h"

class Cef {
 public:
  Cef(int argc, char *argv[]);
  ~Cef();
  // If >= 0, this is a child and not the UI window
  int EarlyExitCode();
  void Tick();

 private:
  int early_exit_code_;

  cef_main_args_t MainArgs(int argc, char *argv[]);
};

#endif // DOOGIE_CEF_H_
