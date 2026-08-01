include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Input/sndfile

HEADERS += decodersndfilefactory.h \
           decoder_sndfile.h \
           sndfilemetadatamodel.h

SOURCES += decoder_sndfile.cpp \
           decodersndfilefactory.cpp \
           sndfilemetadatamodel.cpp

RESOURCES = translations/translations.qrc

unix {
    target.path = $$PLUGIN_DIR/Input
    INSTALLS += target
    PKGCONFIG += taglib sndfile
}

win32 {
    LIBS += -ltag.dll -lsndfile -lflac -lvorbisenc -lvorbis -logg
}
