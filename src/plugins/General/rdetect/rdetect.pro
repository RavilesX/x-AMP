include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/rdetect

HEADERS += rdetectfactory.h \
    rdetectsettingsdialog.h \
           removablehelper.h

SOURCES += rdetectfactory.cpp \
    rdetectsettingsdialog.cpp \
           removablehelper.cpp

FORMS += \
    rdetectsettingsdialog.ui

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

target.path = $$PLUGIN_DIR/General
INSTALLS += target


