#ifndef DOOGIE_CEF_H_
#define DOOGIE_CEF_H_

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4100)
#endif
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#if defined(_MSC_VER)
# pragma warning(pop)
#endif

namespace doogie {

class Cef {
 public:
  Cef(int argc, char* argv[]);
  ~Cef();
  // If >= 0, this is a child and not the UI window
  int EarlyExitCode();
  void Tick();

 private:
  cef_main_args_t MainArgs(int argc, char* argv[]);

  int early_exit_code_;
};

}  // namespace doogie

#endif  // DOOGIE_CEF_H_
