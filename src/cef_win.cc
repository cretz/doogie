#include "cef.h"
#include <Windows.h>

cef_main_args_t Cef::MainArgs(int, char *[]) {
  return cef_main_args_t { GetModuleHandleW(NULL) };
}
