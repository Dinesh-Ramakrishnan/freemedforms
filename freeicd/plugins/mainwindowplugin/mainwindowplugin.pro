TEMPLATE = lib
TARGET = MainWindow
PACKAGE_VERSION = 0.0.2

DEFINES += FREEICD_MAINWIN_LIBRARY
FREEICD = 1

BUILD_PATH_POSTFIXE = FreeICD

include( ../../../plugins/fmf_plugins.pri)
include( mainwindowplugin_dependencies.pri )

INCLUDEPATH += ../
DEPENDPATH += ../

HEADERS += mainwindowplugin.h \
    mainwindow_exporter.h \
    mainwindow.h

SOURCES += mainwindowplugin.cpp \
    mainwindow.cpp

FORMS += mainwindow.ui

# Translators
TRANSLATIONS += $${SOURCES_TRANSLATIONS}/freeicd_mainwindow_fr.ts \
                $${SOURCES_TRANSLATIONS}/freeicd_mainwindow_de.ts \
                $${SOURCES_TRANSLATIONS}/freeicd_mainwindow_es.ts

OTHER_FILES += MainWindow.pluginspec

