include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/listenbrainz

QT += network

HEADERS += listenbrainzfactory.h \
           listenbrainz.h \
           listenbrainzsettingsdialog.h \
           payloadcache.h

SOURCES += listenbrainzfactory.cpp \
           listenbrainz.cpp \
           listenbrainzsettingsdialog.cpp \
           payloadcache.cpp

FORMS += \
    listenbrainzsettingsdialog.ui

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/General
    INSTALLS += target
}
