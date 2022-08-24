#    Copyright (C) 2022 Jesse Daems
#    read the full copyright notice in mainwindow.cpp
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cepresetexportdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    memoryediting.cpp \
    metasettings.cpp \
    oldcepresetimportdialog.cpp \
    savepresetdialog.cpp \
    utils.cpp

HEADERS += \
    cepresetexportdialog.h \
    mainwindow.h \
    metasettings.h \
    oldcepresetimportdialog.h \
    savepresetdialog.h \
    utils.h

FORMS += \
    cepresetexportdialog.ui \
    mainwindow.ui \
    metasettings.ui \
    oldcepresetimportdialog.ui \
    savepresetdialog.ui

RESOURCES += qdarkstyle/dark/style.qrc


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
