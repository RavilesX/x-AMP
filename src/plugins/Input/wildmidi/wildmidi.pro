include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Input/wildmidi

HEADERS += decoderwildmidifactory.h \
    decoder_wildmidi.h \
    wildmidihelper.h \
    wildmidisettingsdialog.h

SOURCES += decoder_wildmidi.cpp \
    decoderwildmidifactory.cpp \
    wildmidihelper.cpp \
    wildmidisettingsdialog.cpp

FORMS += \
    wildmidisettingsdialog.ui

RESOURCES = translations/translations.qrc

PKGCONFIG += wildmidi

target.path = $$PLUGIN_DIR/Input
INSTALLS += target
