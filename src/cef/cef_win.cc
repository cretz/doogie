#include "cef/cef.h"

#include <Windows.h>
#include <cryptuiapi.h>

namespace doogie {

bool Cef::ShowCertDialog(CefRefPtr<CefX509Certificate> cert) const {
  auto top_windows = QGuiApplication::topLevelWindows();
  if (top_windows.isEmpty()) return false;

  // Main struct for the dialog
  CRYPTUI_VIEWCERTIFICATE_STRUCT info = {0};
  info.dwSize = sizeof(info);
  info.hwndParent = reinterpret_cast<HWND>(top_windows.first()->winId());
  info.dwFlags = CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE;

  // In memory store
  // NOTE: do not return early from this function after this because
  // we have to close this.
  auto store = CertOpenStore(
        CERT_STORE_PROV_MEMORY, 0, NULL,
        CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG, NULL);
  if (!store) return false;
  info.cStores = 1;
  info.rghStores = &store;

  // Add primary cert
  info.pCertContext = nullptr;
  auto bytes = CefBinToByteArray(cert->GetDEREncoded());
  auto ok = CertAddEncodedCertificateToStore(
        store, X509_ASN_ENCODING,
        reinterpret_cast<const BYTE*>(bytes.constData()),
        bytes.size(), CERT_STORE_ADD_ALWAYS, &info.pCertContext);
  // Add intermediate certs
  if (ok) {
    std::vector<CefRefPtr<CefBinaryValue>> inters;
    inters.reserve(cert->GetIssuerChainSize());
    cert->GetDEREncodedIssuerChain(inters);
    for (auto& inter : inters) {
      bytes = CefBinToByteArray(inter);
      ok = CertAddEncodedCertificateToStore(
            store, X509_ASN_ENCODING,
            reinterpret_cast<const BYTE*>(bytes.constData()),
            bytes.size(), CERT_STORE_ADD_ALWAYS, NULL);
      if (!ok) break;
    }
  }

  if (ok) {
    BOOL properties_changed;
    CryptUIDlgViewCertificate(&info, &properties_changed);
  }

  CertCloseStore(store, CERT_CLOSE_STORE_FORCE_FLAG);
  return ok;
}

cef_main_args_t Cef::MainArgs(int, char* []) {
  return cef_main_args_t { GetModuleHandleW(NULL) };
}

}  // namespace doogie
