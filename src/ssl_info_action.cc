#include "ssl_info_action.h"

namespace doogie {

SslInfoAction::SslInfoAction(const Cef& cef, BrowserWidget* browser_widg)
    : QWidgetAction(browser_widg), cef_(cef), browser_widg_(browser_widg) { }

QWidget* SslInfoAction::createWidget(QWidget* parent) {
  auto errored_ssl_info = browser_widg_->ErroredSslInfo();
  auto errored_ssl_callback = browser_widg_->ErroredSslCallback();
  auto ssl_status = browser_widg_->SslStatus();
  auto layout = new QVBoxLayout;

  // SSL notes
  cef_cert_status_t status = CERT_STATUS_NONE;
  if (errored_ssl_info) {
    status = errored_ssl_info->GetCertStatus();
  } else if (ssl_status) {
    status = ssl_status->GetCertStatus();
  }
  auto status_strings = CertStatuses(status);
  if (!status_strings.isEmpty()) {
    auto status_layout = new QHBoxLayout;
    status_layout->addWidget(new QLabel("SSL Notes:"), 0, Qt::AlignTop);
    status_layout->addWidget(new QLabel(status_strings.join("\n")), 1);
    layout->addLayout(status_layout);
  }

  // Version and insecure content
  if (ssl_status) {
    auto version_layout = new QHBoxLayout;
    version_layout->addWidget(new QLabel("SSL Version:"));
    version_layout->addWidget(
        new QLabel(CertSslVersion(ssl_status->GetSSLVersion())), 1);
    layout->addLayout(version_layout);

    if (ssl_status->GetContentStatus() != SSL_CONTENT_NORMAL_CONTENT) {
      layout->addWidget(new QLabel("Displayed or ran insecure content"));
    }
  }

  // TODO(cretz): allow bypass

  // The button to view OS-level dialog
  auto details_button = new QPushButton("View Certificate Details");
  connect(details_button, &QPushButton::clicked, [=](bool) {
    bool ok = false;
    if (errored_ssl_info) {
      ok = cef_.ShowCertDialog(errored_ssl_info->GetX509Certificate());
    } else if (ssl_status) {
      ok = cef_.ShowCertDialog(ssl_status->GetX509Certificate());
    }
    if (!ok) qWarning() << "Failed to show native cert dialog";
  });
  layout->addWidget(details_button, 0, Qt::AlignCenter);

  layout->addStretch(1);
  auto widg = new QWidget(parent);
  widg->setLayout(layout);
  return widg;
}

QStringList SslInfoAction::CertStatuses(cef_cert_status_t status) {
  const QHash<cef_cert_status_t, QString> strings = {
    { CERT_STATUS_COMMON_NAME_INVALID, "Common Name Invalid" },
    { CERT_STATUS_DATE_INVALID, "Date Invalid" },
    { CERT_STATUS_AUTHORITY_INVALID, "Authority Invalid" },
    { CERT_STATUS_NO_REVOCATION_MECHANISM, "No Revocation Mechanism" },
    { CERT_STATUS_UNABLE_TO_CHECK_REVOCATION, "Unable to Check Revocation" },
    { CERT_STATUS_REVOKED, "Revoked" },
    { CERT_STATUS_INVALID, "Invalid" },
    { CERT_STATUS_WEAK_SIGNATURE_ALGORITHM, "Weak Signature Algorithm" },
    { CERT_STATUS_NON_UNIQUE_NAME, "Non-Unique Name" },
    { CERT_STATUS_WEAK_KEY, "Weak Key" },
    { CERT_STATUS_PINNED_KEY_MISSING, "Pinned Key Missing" },
    { CERT_STATUS_NAME_CONSTRAINT_VIOLATION, "Name Constraint Violation" },
    { CERT_STATUS_VALIDITY_TOO_LONG, "Validity Too Long" },
    { CERT_STATUS_IS_EV, "Extended Validation Certificate" },
    { CERT_STATUS_REV_CHECKING_ENABLED, "Revocation Checking Enabled" },
    { CERT_STATUS_SHA1_SIGNATURE_PRESENT, "SHA1 Signature Present" },
    { CERT_STATUS_CT_COMPLIANCE_FAILED, "CT Compliance Failed" }
  };
  QStringList ret;
  for (auto key : strings.keys()) {
    if (status & key) ret << strings[key];
  }
  return ret;
}

QString SslInfoAction::CertSslVersion(cef_ssl_version_t ssl_version) {
  switch (ssl_version) {
    case SSL_CONNECTION_VERSION_SSL2: return "SSL 2.0";
    case SSL_CONNECTION_VERSION_SSL3: return "SSL 3.0";
    case SSL_CONNECTION_VERSION_TLS1: return "TLS 1.0";
    case SSL_CONNECTION_VERSION_TLS1_1: return "TLS 1.1";
    case SSL_CONNECTION_VERSION_TLS1_2: return "TLS 1.2";
    case SSL_CONNECTION_VERSION_QUIC: return "QUIC";
    default: return "(unknown)";
  }
}

}  // namespace doogie
