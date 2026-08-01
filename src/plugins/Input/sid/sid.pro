include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Input/sid

HEADERS += decodersidfactory.h \
    decoder_sid.h \
    sidhelper.h \
    sidsettingsdialog.h

SOURCES += decoder_sid.cpp \
    decodersidfactory.cpp \
    sidhelper.cpp \
    sidsettingsdialog.cpp

RESOURCES = translations/translations.qrc

FORMS += \
    sidsettingsdialog.ui

DEFINES += HAVE_RESIDFP_HEADER

unix{
    target.path = $$PLUGIN_DIR/Input
    INSTALLS += target
    PKGCONFIG += libsidplayfp
}

win32 {
    LIBS += -lsidplayfp.dll
}
