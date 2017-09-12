#ifndef DOOGIE_SSL_INFO_WIDGET_H_
#define DOOGIE_SSL_INFO_WIDGET_H_

#include <QtWidgets>

#include "browser_widget.h"
#include "cef/cef.h"

namespace doogie {

// Customized action to show SSL details on lock-icon click.
class SslInfoAction : public QWidgetAction {
  Q_OBJECT

 public:
  explicit SslInfoAction(
      const Cef& cef,
      BrowserWidget* browser_widg_);

 protected:
  QWidget* createWidget(QWidget* parent) override;

 private:
  static QStringList CertStatuses(cef_cert_status_t status);
  static QString CertSslVersion(cef_ssl_version_t ssl_version);

  const Cef& cef_;
  BrowserWidget* browser_widg_;
};

}  // namespace doogie

#endif  // DOOGIE_SSL_INFO_WIDGET_H_
