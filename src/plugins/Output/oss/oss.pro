include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Output/oss

FORMS += \
    osssettingsdialog.ui

HEADERS += outputossfactory.h \
           osssettingsdialog.h \
           outputoss.h

SOURCES += outputossfactory.cpp \
           osssettingsdialog.cpp \
           outputoss.cpp

RESOURCES = translations/translations.qrc

DEFINES += HAVE_SYS_SOUNDCARD_H

target.path = $$PLUGIN_DIR/Output
INSTALLS += target
