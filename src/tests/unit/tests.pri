
test {
    QT += testlib
    # Have to remove main
    SOURCES -= main.cc
    SOURCES += \
        tests/unit/blocker_rules.test.cc
    TARGET = doogie-test
}

benchmark {
    QT += testlib network
    # Have to remove main
    SOURCES -= main.cc
    SOURCES += \
        tests/unit/blocker_rules.benchmark.cc
    TARGET = doogie-benchmark
}
