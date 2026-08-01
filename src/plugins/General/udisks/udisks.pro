include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/udisks

QT += dbus

HEADERS += udisksfactory.h \
           udisksplugin.h \
           udisksmanager.h \
           udisksdevice.h \
           udiskssettingsdialog.h

SOURCES += udisksfactory.cpp \
           udisksplugin.cpp \
           udisksmanager.cpp \
           udisksdevice.cpp \
           udiskssettingsdialog.cpp

FORMS += \
    udiskssettingsdialog.ui

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

target.path = $$PLUGIN_DIR/General
INSTALLS += target
