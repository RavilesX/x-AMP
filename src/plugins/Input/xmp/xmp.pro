include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Input/xmp

HEADERS += decoderxmpfactory.h \
    decoder_xmp.h \
    xmpmetadatamodel.h \
    xmpsettingsdialog.h

SOURCES += decoder_xmp.cpp \
    decoderxmpfactory.cpp \
    xmpmetadatamodel.cpp \
    xmpsettingsdialog.cpp

FORMS += \
    xmpsettingsdialog.ui

RESOURCES = translations/translations.qrc

unix {
    target.path = $$PLUGIN_DIR/Input
    INSTALLS += target
    PKGCONFIG += libxmp
    QMAKE_CLEAN = $$PLUGINS_PREFIX/Input/libxmp.so
}

win32 {
    LIBS += -lxmp
}
