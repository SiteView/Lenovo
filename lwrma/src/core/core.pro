VAR_DEVEL_ROOT_DIR = $$PWD/../..
VAR_BUILD_ROOT_DIR = $$OUT_PWD/../..
DESTDIR = $$VAR_BUILD_ROOT_DIR/debug

build_pass:CONFIG(debug, debug|release) {
	DESTDIR = $$VAR_BUILD_ROOT_DIR/debug
	VAR_PUBLIC_LIB_DIR = $$VAR_BUILD_ROOT_DIR/debug
}
else: build_pass: {
	DESTDIR = $$VAR_BUILD_ROOT_DIR/release
	VAR_PUBLIC_LIB_DIR = $$VAR_BUILD_ROOT_DIR/release
}

TEMPLATE = lib
TARGET = LenovoCore
CONFIG += precompile_header staticlib
PRECOMPILED_HEADER = impl/pch.h
QT += network test

INCLUDEPATH += $$VAR_DEVEL_ROOT_DIR/src/core/public \
	./
DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII LENOVOCORE_STATICLIB

HEADERS += \
	public/LenovoCore/AsyncOp.h \
	public/LenovoCore/Base.h \
	public/LenovoCore/Bean.h \
	public/LenovoCore/LenovoCore.h \
	public/LenovoCore/Logger.h \
	public/LenovoCore/SoapCore.h \
	public/LenovoCore/System.h \
	impl/AsyncOpImpl.h \
	impl/BeanImpl.h \
	impl/LoggerImpl.h \
	impl/SoapCoreImpl.h \
	impl/SystemImpl.h \
	impl/wlandef.h \
	impl/wlanlib.h \
	impl/misc.h \
	impl/pch.h

SOURCES += \
	impl/AsyncOp.cpp \
	impl/AsyncOpImpl.cpp \
	impl/Bean.cpp \
	impl/BeanImpl.cpp \
	impl/Logger.cpp \
	impl/LoggerImpl.cpp \
	impl/SoapCore.cpp \
	impl/SoapCoreImpl.cpp \
	impl/System.cpp \
	impl/SystemImpl.cpp \
	impl/wlanlib.cpp \
	impl/misc.cpp
