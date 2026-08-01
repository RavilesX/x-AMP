include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Input/aac

HEADERS += decoderaacfactory.h \
    decoder_aac.h \
    aacfile.h
SOURCES += decoder_aac.cpp \
    decoderaacfactory.cpp \
    aacfile.cpp

RESOURCES = translations/translations.qrc

PKGCONFIG += taglib faad2

target.path = $$PLUGIN_DIR/Input
INSTALLS += target
