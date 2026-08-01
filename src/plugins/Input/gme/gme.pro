include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Input/gme

HEADERS += decodergmefactory.h \
    decoder_gme.h \
    gmehelper.h \
    gmesettingsdialog.h

SOURCES += decoder_gme.cpp \
    decodergmefactory.cpp \
    gmehelper.cpp \
    gmesettingsdialog.cpp

FORMS += \
    gmesettingsdialog.ui

RESOURCES = translations/translations.qrc

unix{
    target.path = $$PLUGIN_DIR/Input
    INSTALLS += target
    PKGCONFIG += libgme
}

win32 {
    LIBS += -lgme.dll
}
