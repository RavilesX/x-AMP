include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Output/oss4

HEADERS += outputoss4factory.h \
           oss4settingsdialog.h \
           outputoss4.h

SOURCES += outputoss4factory.cpp \
           oss4settingsdialog.cpp \
           outputoss4.cpp

FORMS += \
    oss4settingsdialog.ui

RESOURCES = translations/translations.qrc

DEFINES += HAVE_SYS_SOUNDCARD_H

target.path = $$PLUGIN_DIR/Output
INSTALLS += target
