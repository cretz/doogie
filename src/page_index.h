#ifndef DOOGIE_PAGE_INDEX_H_
#define DOOGIE_PAGE_INDEX_H_

#include <QtSql>
#include <QtWidgets>

namespace doogie {

// Utility/helper class for doing page index lookups for things like
// autocomplete.
class PageIndex {
 public:
  class Expirer {
   public:
    Expirer();
   private:
    QTimer timer;
  };

  struct AutocompletePage {
    QString url;
    QString title;
    QIcon favicon;
  };

  // Just make it 90 days for now
  static const qlonglong kExpireNotVisitedSinceSeconds = 90ll * 24 * 60 * 60;
  // Amount of time a visit is worth for frecency...for now it's a day
  static const qlonglong kVisitTimeWorthSeconds = 24ll * 60 * 60;

  static QList<AutocompletePage> AutocompleteSuggest(const QString& text,
                                                     int count);

  static bool MarkVisit(const QString& url,
                        const QString& title,
                        const QString& favicon_url,
                        const QIcon& favicon);
  static bool UpdateTitle(const QString& url, const QString& title);
  static bool UpdateFavicon(const QString& url,
                            const QString& favicon_url,
                            const QIcon& favicon);

  static QIcon CachedFavicon(const QString& url);

 private:
  static void DoExpiration();

  static QVariant FaviconId(const QString& url, const QIcon& favicon);
};

}  // namespace doogie

#endif  // DOOGIE_PAGE_INDEX_H_
