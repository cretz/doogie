#include <QtNetwork>
#include <QtTest>
#include <QtWidgets>

#include "blocker_rules.h"

namespace doogie {

class BlockerRulesBenchmark : public QObject {
  Q_OBJECT

 private:
  void EnsureEasyListLoaded() {
    // Only wait 10 seconds
    LoadEasyList();
    QTRY_VERIFY_WITH_TIMEOUT(!error_.isNull() || !easy_list_.isNull(), 10000);
    QVERIFY(error_.isNull());
  }

  void LoadEasyList() {
    if (!easy_list_.isNull()) return;
    LoadEasyListFromFile();
    if (!easy_list_.isNull()) return;
    LoadEasyListFromInternet();
  }

  QString EasyListTempFilePath() const {
    return QDir(QStandardPaths::writableLocation(
        QStandardPaths::TempLocation)).filePath("easylist.txt");
  }

  void LoadEasyListFromFile() {
    QFile file(EasyListTempFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;
    easy_list_ = file.readAll();
  }

  void LoadEasyListFromInternet() {
    if (net_mgr_) net_mgr_->deleteLater();
    net_mgr_ = new QNetworkAccessManager(this);
    connect(net_mgr_, &QNetworkAccessManager::finished,
            [=](QNetworkReply* reply) {
      if (reply->error() != QNetworkReply::NoError) {
        error_ = reply->errorString();
      } else {
        easy_list_ = reply->readAll();
        SaveEasyListToFile();
      }
      reply->deleteLater();
    });
    net_mgr_->get(QNetworkRequest(
        QUrl("https://easylist.to/easylist/easylist.txt")));
  }

  void SaveEasyListToFile() {
    QFile file(EasyListTempFilePath());
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(easy_list_);
  }

  void SaveEasyListToJson() {
    auto path = QDir(QStandardPaths::writableLocation(
        QStandardPaths::TempLocation)).filePath("easylist.json");
    QFile file(path);
    if (file.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
      file.write(QJsonDocument(easy_list_rules_->RuleTree()).toJson());
    }
    qDebug() << "Saved JSON to" << path;
  }

  // Auto deleted at end of each test case
  BlockerRules* ParsedRules(const QByteArray& data, int file_index = 1) {
    cleanupLastRules();
    QTextStream stream(data, QIODevice::ReadOnly);
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

  QNetworkAccessManager* net_mgr_ = nullptr;
  QString error_;
  QByteArray easy_list_;
  BlockerRules* last_rules_ = nullptr;
  BlockerRules* easy_list_rules_ = nullptr;

 private slots:  // NOLINT(whitespace/indent)
  void initTestCase() {
    EnsureEasyListLoaded();
    // Some sleeps to observe memory...
    // qDebug() << "Sleeping before parse...";
    // QTest::qSleep(5000);

    easy_list_rules_ = ParsedRules(easy_list_);
    // Remove it from last_rules_ so it doesn't get auto-deleted
    last_rules_ = nullptr;

    // qDebug() << "Sleeping after parse...";
    // QTest::qSleep(5000);
  }

  void cleanupTestCase() {
    if (easy_list_rules_) {
      delete easy_list_rules_;
      easy_list_rules_ = nullptr;
    }
  }

  void cleanup() {
    cleanupLastRules();
  }

  void benchmarkParse() {
    QBENCHMARK {
      ParsedRules(easy_list_);
    }
  }

  void benchmarkSimpleUrl() {
    BlockerRules::StaticRule::FindResult* rule = nullptr;
    QBENCHMARK {
      rule = easy_list_rules_->FindStaticRule(
            "http://example.com/foo/bar/-adserver-/baz",
            "http://example.com",
            BlockerRules::StaticRule::Image);
    }
    QVERIFY(rule);
  }
};

}  // namespace doogie

QTEST_MAIN(doogie::BlockerRulesBenchmark)
#include "blocker_rules.benchmark.moc"
