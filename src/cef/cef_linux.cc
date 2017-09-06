#include "cef/cef.h"

namespace doogie {

bool Cef::ShowCertDialog(CefRefPtr<CefX509Certificate> /*cert*/) const {
  // TODO(cretz): Implement this
  QMessageBox::warning(nullptr, "Show Certificate",
                       "Certificate dialog not yet implemented on Linux");
  return false;
}

cef_main_args_t Cef::MainArgs(int argc, char* argv[]) {
  return cef_main_args_t { argc, argv };
}

}  // namespace doogie
