include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Output/alsa

HEADERS += outputalsa.h \
           alsasettingsdialog.h \
           outputalsafactory.h
SOURCES += outputalsa.cpp \
           alsasettingsdialog.cpp \
           outputalsafactory.cpp

FORMS += \
    alsasettingsdialog.ui

RESOURCES = translations/translations.qrc

PKGCONFIG += alsa

target.path = $$PLUGIN_DIR/Output
INSTALLS += target
