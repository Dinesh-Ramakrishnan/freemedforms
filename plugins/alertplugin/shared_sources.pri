TEMPLATE = lib
TARGET = Alert

DEFINES += ALERT_LIBRARY

QT += sql

include(../fmf_plugins.pri)
include(alertplugin_dependencies.pri)

HEADERS += \
    $${PWD}/constants.h \
    $${PWD}/alertplugin.h \
    $${PWD}/alertbase.h \
    $${PWD}/alertcore.h \
    $${PWD}/alertmanager.h \
    $${PWD}/alertitem.h \
    $${PWD}/showalertsIHM.h \
    $${PWD}/alertmodel.h \
    $${PWD}/ialertplaceholder.h \
    $${PWD}/alertitemeditorwidget.h \
    $${PWD}/alertitemtimingeditorwidget.h \
    $${PWD}/alertitemeditordialog.h \
    $${PWD}/dynamicalertdialog.h \
    $${PWD}/alertplaceholdertest.h \
    $${PWD}/staticalertwidgets.h

SOURCES += \
    $${PWD}/alertplugin.cpp \
    $${PWD}/alertbase.cpp \
    $${PWD}/alertcore.cpp \
    $${PWD}/alertmanager.cpp \
    $${PWD}/alertitem.cpp \
    $${PWD}/showalertsIHM.cpp \
    $${PWD}/alertmodel.cpp \
    $${PWD}/ialertplaceholder.cpp \
    $${PWD}/alertitemeditorwidget.cpp \
    $${PWD}/alertitemtimingeditorwidget.cpp \
    $${PWD}/alertitemeditordialog.cpp \
    $${PWD}/dynamicalertdialog.cpp \
    $${PWD}/alertplaceholdertest.cpp \
    $${PWD}/staticalertwidgets.cpp

FORMS += $${PWD}/ShowAlertsWidget.ui \
    $${PWD}/alertitemeditorwidget.ui \
    $${PWD}/alertitemtimingeditorwidget.ui \
    $${PWD}/alertitemeditordialog.ui \
    $${PWD}/dynamicalertdialog.ui \
    $${PWD}/dynamicalertdialogoverridingcomment.ui

OTHER_FILES += $${PWD}/Alert.pluginspec

TRANSLATIONS += \
    $${SOURCES_TRANSLATIONS_PATH}/alertplugin_fr.ts \
    $${SOURCES_TRANSLATIONS_PATH}/alertplugin_de.ts \
    $${SOURCES_TRANSLATIONS_PATH}/alertplugin_es.ts
