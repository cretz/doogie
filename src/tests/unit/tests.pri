
test {
    QT += testlib
    # Have to remove main
    SOURCES -= main.cc
    SOURCES += \
        tests/unit/blocker_rules.test.cc
    TARGET = doogie-test
}
