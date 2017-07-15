#include "cef.h"

namespace doogie {

cef_main_args_t Cef::MainArgs(int argc, char* argv[]) {
  return cef_main_args_t { argc, argv };
}

}  // namespace doogie
