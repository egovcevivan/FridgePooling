QT       += core gui
QT       += core network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    fixedlineedit.cpp \
    main.cpp \
    prods.cpp \
    recipes.cpp \
    web_parsing.cpp \
    window.cpp

HEADERS += \
    fixedlineedit.h \
    json.hpp \
    prods.h \
    recipes.h \
    web_parsing.h \
    window.h

FORMS += \
    window.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QMAKE_CXXFLAGS_RELEASE += -g
QMAKE_CFLAGS_RELEASE += -g
