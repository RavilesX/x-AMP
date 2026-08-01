include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Effect/filewriter

HEADERS += effectfilewriterfactory.h \
           filewriterplugin.h \
           filewritersettingsdialog.h

SOURCES += effectfilewriterfactory.cpp \
           filewriterplugin.cpp \
           filewritersettingsdialog.cpp

FORMS += \
    filewritersettingsdialog.ui

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/Effect
    INSTALLS += target
    PKGCONFIG += ogg vorbis vorbisenc
}

win32 {
    LIBS += -lvorbisfile -lvorbis -logg -lvorbisenc
    LD_FLAGS += -no-undefined
}
