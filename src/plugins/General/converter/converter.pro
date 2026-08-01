include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/converter

HEADERS += converterfactory.h \
    converterhelper.h \
    converterdialog.h \
    converter.h \
    converterpreseteditor.h

SOURCES += converterfactory.cpp \
    converterhelper.cpp \
    converterdialog.cpp \
    converter.cpp \
    converterpreseteditor.cpp

FORMS += converterdialog.ui \
    converterpreseteditor.ui

RESOURCES = translations/translations.qrc presets.qrc

LIBS += $$QMMPUI_LIB
PKGCONFIG += taglib

unix{
    target.path = $$PLUGIN_DIR/General
    INSTALLS += target
}
