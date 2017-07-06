#include "cef.h"

cef_main_args_t Cef::MainArgs(int argc, char *argv[]) {
  return cef_main_args_t { argc, argv };
}
