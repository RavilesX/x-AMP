include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/scrobbler

QT += network

HEADERS += scrobblerfactory.h \
    scrobblerhandler.h \
    scrobblercache.h \
    scrobbler.h \
    defines.h \
    scrobblersettingsdialog.h

SOURCES += scrobblerfactory.cpp \
    scrobblerhandler.cpp \
    scrobblercache.cpp \
    scrobbler.cpp \
    scrobblersettingsdialog.cpp

FORMS += \
    scrobblersettingsdialog.ui

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/General
    INSTALLS += target
}
