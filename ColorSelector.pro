QT       += core gui

!versionAtLeast(QT_VERSION, 6.0.0) {
    error("This project requires Qt 6.0 or newer. Current version: $$QT_VERSION")
}

QT += widgets

QMAKE_CXXFLAGS += -Wno-reorder -Wno-unused-function
# CONFIG += force_debug_info  # This is the key line for Qt
CONFIG += c++17
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_LFLAGS_RELEASE -= -O1

QMAKE_CXXFLAGS_DEBUG -= -O1
QMAKE_CXXFLAGS_DEBUG -= -O2
QMAKE_CXXFLAGS_DEBUG -= -O3
QMAKE_CXXFLAGS_DEBUG += -O0
QMAKE_CXXFLAGS += -Wfatal-errors

CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS_RELEASE += -march=native -mtune=native \
                              -flto -funroll-loops -ftree-vectorize \
                              -funroll-loops -ftree-vectorize \
                              -g -Og \                  # good balance
                              -fno-math-errno -funsafe-math-optimizations -fno-signed-zeros
    # Deliberately omit -ffast-math and -ffinite-math-only
    QMAKE_LFLAGS_RELEASE += -flto
}

DEFINES += QT_NO_CAST_FROM_ASCII \
           QT_NO_CAST_TO_ASCII

# CONFIG(release, debug|release):DEFINES -= QT_NO_DEBUG_OUTPUT

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    gradient_demo_widget.cpp \
    main.cpp \
    unit_test.cpp \
	$$files(color_select/*.cpp)

HEADERS += \
    gradient_demo_widget.h \
    unit_test.h \
    $$files(color_select/*.h)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
   README.TXT
