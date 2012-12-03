VAR_DEVEL_ROOT_DIR = $$PWD/../..
VAR_BUILD_ROOT_DIR = $$OUT_PWD/../..
DESTDIR = $$VAR_BUILD_ROOT_DIR/debug
build_pass:CONFIG(debug, debug|release) { 
    DESTDIR = $$VAR_BUILD_ROOT_DIR/debug
    VAR_PUBLIC_LIB_DIR = $$VAR_BUILD_ROOT_DIR/debug
}
else:build_pass: { 
    DESTDIR = $$VAR_BUILD_ROOT_DIR/release
    VAR_PUBLIC_LIB_DIR = $$VAR_BUILD_ROOT_DIR/release
}
TEMPLATE = app
TARGET = lwrma
RESOURCES = app.qrc
QMAKE_RESOURCE_FLAGS += -threshold 0 -compress 9
CONFIG += precompile_header \
    uitools
PRECOMPILED_HEADER = pch.h
QT += network
INCLUDEPATH += $$VAR_DEVEL_ROOT_DIR/src/core/public \
	./
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII LENOVOCORE_STATICLIB

HEADERS += AppPage.h \
    AppService.h \
    AppUILoader.h \
    MiniApp.h \
    pages.h \
    pch.h \
    #MainFrame.h \
    #uiconfigurewirelessfinish.h \
    #uiconfigurewireless.h \
    #uiconfigurepasswordforwirless.h \
    #uiconfigureispip.h \
    #uiconfigureaccountandpassword.h \
    #DetailUiHelper.h
    MainFrame.h \
    DetailUiHelper.h
	
SOURCES += AppPage.cpp \
    AppService.cpp \
    AppUILoader.cpp \
    MiniApp.cpp \
    pages.cpp \
    main.cpp \
    #MainFrame.cpp \
    #uiconfigurewirelessfinish.cpp \
    #uiconfigurewireless.cpp \
    #uiconfigurepasswordforwirless.cpp \
    #uiconfigureispip.cpp \
    #uiconfigureaccountandpassword.cpp \
    #DetailUiHelper.cpp
    MainFrame.cpp \
    DetailUiHelper.cpp
	
win32:{
	RC_FILE = app.rc
}	

win32-msvc*: { 
    DEFINES += DEV_SRC_ROOT=\"$$PWD\"
    LIBS += $$VAR_PUBLIC_LIB_DIR/LenovoCore.lib
}
win32-g++*: { 
    DEFINES += DEV_SRC_ROOT=\\\"$$PWD\\\"
    LIBS += -L$$VAR_PUBLIC_LIB_DIR \
        -lLenovoCore
}
DEFINES += DETAIL_UI
