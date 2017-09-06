#include <QtTest>
#include <QtWidgets>

#include "blocker_rules.h"

namespace doogie {

class BlockerRulesTest : public QObject {
  Q_OBJECT

 private:
  // Auto deleted at end of each test case
  BlockerRules* ParsedRules(const QString& text, int file_index = 1) {
    cleanupLastRules();
    QString text_str(text);
    QTextStream stream(&text_str);
    last_rules_ = new BlockerRules;
    last_rules_->AddRules(&stream, file_index);
    return last_rules_;
  }

  void cleanupLastRules() {
    if (last_rules_) {
      delete last_rules_;
      last_rules_ = nullptr;
    }
  }

  BlockerRules* last_rules_ = nullptr;

 private slots:  // NOLINT(whitespace/indent)
  void cleanup() {
    cleanupLastRules();
  }

  void testSimpleStaticRules() {
    auto rules = ParsedRules(
        "&adbannerid=\n"
        "/ads/profile/*\n"
        "/advert-$domain=~advert-technology.com|~advert-technology.ru\n"
        "||7pud.com^$third-party\n"
        "@@||speedtest.net^*/results.php$xmlhttprequest",
        0);
    qDebug().noquote() << "JSON:\n" <<
                          QJsonDocument(rules->RuleTree()).toJson();
    auto rule = rules->FindStaticRule(
          "http://example.com/?foo=bar&adbannerid=35",
          "http://example.com/",
          BlockerRules::StaticRule::AllRequests);
    QVERIFY(rule && rule->info.line_num == 1);
    rule = rules->FindStaticRule(
          "http://example.com/foo/ads/profile/bar",
          "http://example.com/",
          BlockerRules::StaticRule::AllRequests);
    QVERIFY(rule && rule->info.line_num == 2);
    rule = rules->FindStaticRule(
          "http://example.com/advert-whatever",
          "http://advert-technology.com/",
          BlockerRules::StaticRule::AllRequests);
    QVERIFY(!rule);
    rule = rules->FindStaticRule(
          "http://example.com/advert-whatever",
          "http://example.com/",
          BlockerRules::StaticRule::AllRequests);
    QVERIFY(rule && rule->info.line_num == 3);
    rule = rules->FindStaticRule(
          "http://foo.7pud.com/whatever",
          "http://example.com/",
          BlockerRules::StaticRule::AllRequests);
    QVERIFY(rule && rule->info.line_num == 4);
    rule = rules->FindStaticRule(
          "http://foo.7pud.com/whatever",
          "http://foo.7pud.com/",
          BlockerRules::StaticRule::AllRequests);
    QVERIFY(!rule);
    rule = rules->FindStaticRule(
          "http://speedtest.net/ads/profile/results.php",
          "http://speedtest.net/",
          BlockerRules::StaticRule::AllRequests);
    QVERIFY(rule && rule->info.line_num == 2);
    rule = rules->FindStaticRule(
          "http://speedtest.net/ads/profile/results.php",
          "http://speedtest.net/",
          BlockerRules::StaticRule::XmlHttpRequest);
    QVERIFY(!rule);
  }
};

}  // namespace doogie

QTEST_MAIN(doogie::BlockerRulesTest)
#include "blocker_rules.test.moc"
