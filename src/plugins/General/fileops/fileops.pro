include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/fileops

HEADERS += fileopsfactory.h \
           fileops.h \
           fileopssettingsdialog.h

SOURCES += fileopsfactory.cpp \
           fileops.cpp \
           fileopssettingsdialog.cpp

FORMS += \
         fileopssettingsdialog.ui

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/General
    INSTALLS += target
}
