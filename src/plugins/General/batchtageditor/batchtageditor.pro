include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/batchtageditor

HEADERS += batchtageditorfactory.h \
           batchtageditor.h \
           batchtageditordialog.h

SOURCES += batchtageditorfactory.cpp \
           batchtageditor.cpp \
           batchtageditordialog.cpp

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/General
    INSTALLS += target
}

FORMS += \
    batchtageditordialog.ui
