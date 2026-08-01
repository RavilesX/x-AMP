include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Output/shout

HEADERS += outputshoutfactory.h \
    shoutoutput.h \
    shoutclient.h \
    shoutsettingsdialog.h

SOURCES += outputshoutfactory.cpp \
    shoutoutput.cpp \
    shoutclient.cpp \
    shoutsettingsdialog.cpp

FORMS += \
    shoutsettingsdialog.ui

RESOURCES = translations/translations.qrc

unix {
    target.path = $$PLUGIN_DIR/Output
    INSTALLS += target
    PKGCONFIG += ogg vorbis vorbisenc shout soxr
}

