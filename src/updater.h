#ifndef DOOGIE_UPDATER_H_
#define DOOGIE_UPDATER_H_

#include <QtWidgets>

#include "cef/cef.h"
#include "page_index.h"

namespace doogie {

// Class to regularly update things in the background.
class Updater : public QObject {
  Q_OBJECT
 public:
  explicit Updater(const Cef& cef, QObject* parent = nullptr);

 private:
  static const int kCrlUpdateFailRetrySeconds = 300;
  static const int kCheckCrlUpdateFrequencySeconds = 5 * 3600;
  static const QString kCrlCheckUrl;

  void ApplyCrlFromFileAndScheduleUpdate() const;
  QString CrlFilePath() const;
  void CheckCrlUpdates() const;
  void UpdateCrl(const QString& crx_url) const;

  const Cef& cef_;
  // This auto-creates this which starts its timer
  PageIndex::Expirer expirer_;
};

}  // namespace doogie

#endif  // DOOGIE_UPDATER_H_
