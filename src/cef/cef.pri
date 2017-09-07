
SOURCES += \
    cef/cef.cc \
    cef/cef_app_handler.cc \
    cef/cef_base_widget.cc \
    cef/cef_handler.cc \
    cef/cef_widget.cc

HEADERS += \
    cef/cef.h \
    cef/cef_app_handler.h \
    cef/cef_base.h \
    cef/cef_base_widget.h \
    cef/cef_handler.h \
    cef/cef_widget.h \

INCLUDEPATH += $$(CEF_DIR)

win32 {
    SOURCES += \
        cef/cef_base_widget_win.cc \
        cef/cef_win.cc

    LIBS += -L$$(CEF_DIR)/$$PROFILE -llibcef
    LIBS += -L$$(CEF_DIR)/libcef_dll_wrapper/$$PROFILE -llibcef_dll_wrapper
}

unix {
    SOURCES += \
        cef/cef_base_widget_linux.cc \
        cef/cef_embed_window_linux.cc \
        cef/cef_linux.cc
    HEADERS += \
        cef/cef_embed_window_linux.h
    LIBS += -L$$(CEF_DIR)/$$PROFILE -lcef
    LIBS += -L$$(CEF_DIR)/libcef_dll_wrapper -lcef_dll_wrapper_$$PROFILE
}
