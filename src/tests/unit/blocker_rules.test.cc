#include <QtTest>
#include <QtWidgets>

namespace doogie {

class BlockerRulesTest : public QObject {
  Q_OBJECT

 private slots:  // NOLINT(whitespace/indent)
  void testSimple();
};

void BlockerRulesTest::testSimple() {
  QCOMPARE(1, 1);
}

}  // namespace doogie

QTEST_MAIN(doogie::BlockerRulesTest)
#include "blocker_rules.test.moc"
