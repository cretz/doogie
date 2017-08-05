#include "cef/cef.h"

#include <Windows.h>

namespace doogie {

cef_main_args_t Cef::MainArgs(int, char* []) {
  return cef_main_args_t { GetModuleHandleW(NULL) };
}

}  // namespace doogie
