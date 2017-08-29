
QT += core gui widgets sql
TARGET = doogie
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++14

# We want a console window for logs during debug
debug:CONFIG += console

# Keep all sources and headers in alphabetical order

SOURCES += \
    action_manager.cc \
    blocker_dock.cc \
    blocker_list.cc \
    blocker_rules.cc \
    browser_setting.cc \
    browser_stack.cc \
    browser_widget.cc \
    bubble.cc \
    bubble_settings_dialog.cc \
    cosmetic_blocker.cc \
    dev_tools_dock.cc \
    download.cc \
    downloads_dock.cc \
    download_list_item.cc \
    find_widget.cc \
    logging_dock.cc \
    main.cc \
    main_window.cc \
    page_index.cc \
    page_tree.cc \
    page_tree_dock.cc \
    page_tree_item.cc \
    profile.cc \
    profile_change_dialog.cc \
    profile_settings_dialog.cc \
    settings_widget.cc \
    sql.cc \
    url_edit.cc \
    util.cc \
    workspace.cc \
    workspace_dialog.cc \
    workspace_tree_item.cc

HEADERS += \
    action_manager.h \
    blocker_dock.h \
    blocker_list.h \
    blocker_rules.h \
    browser_setting.h \
    browser_stack.h \
    browser_widget.h \
    bubble.h \
    bubble_settings_dialog.h \
    cosmetic_blocker.h \
    dev_tools_dock.h \
    download.h \
    downloads_dock.h \
    download_list_item.h \
    find_widget.h \
    logging_dock.h \
    main_window.h \
    page_index.h \
    page_tree.h \
    page_tree_dock.h \
    page_tree_item.h \
    profile.h \
    profile_change_dialog.h \
    profile_settings_dialog.h \
    settings_widget.h \
    sql.h \
    url_edit.h \
    util.h \
    workspace.h \
    workspace_dialog.h \
    workspace_tree_item.h

RESOURCES += \
    doogie.qrc

# We need web sockets for debug meta server
debug:QT += websockets
debug:SOURCES += debug_meta_server.cc
debug:HEADERS += debug_meta_server.h

release:PROFILE = Release
debug:PROFILE = Debug

win32 {
    LIBS += -luser32

    SOURCES += \
        util_win.cc

    # Chromium reads the manifest, needs a specific one
    # See http://magpcss.org/ceforum/viewtopic.php?f=6&t=14721
    CONFIG -= embed_manifest_exe
    RC_FILE = doogie.rc

    # We have to ignore this compiler warning because MSVC
    #  complains about > 255 type name lengths in debug which
    #  we don't care about.
    QMAKE_CXXFLAGS += /wd4503
}

unix {
    LIBS += -lX11
    QMAKE_RPATHDIR += $ORIGIN

    release:MOC_DIR = release
    release:OBJECTS_DIR = release
    release:DESTDIR = release
    debug:MOC_DIR = debug
    debug:OBJECTS_DIR = debug
    debug:DESTDIR = debug
}

# include cef folder
include(cef/cef.pri)

# include vendor folder
include(vendor/vendor.pri)

# include tests
include(tests/unit/tests.pri)
