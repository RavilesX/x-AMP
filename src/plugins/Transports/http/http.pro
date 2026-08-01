include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Transports/http

HEADERS += \
    httpinputfactory.h \
    httpinputsource.h \
    httpsettingsdialog.h \
    httpstreamreader.h

SOURCES += \
    httpinputfactory.cpp \
    httpinputsource.cpp \
    httpsettingsdialog.cpp \
    httpstreamreader.cpp

FORMS += \
    httpsettingsdialog.ui

RESOURCES = translations/translations.qrc

contains(CONFIG, WITH_ENCA){
   DEFINES += WITH_ENCA
   unix:PKGCONFIG += enca
   win32:LIBS += -lenca.dll
}

unix {
    LIBS += -L/usr/lib
    PKGCONFIG += libcurl
    target.path = $$PLUGIN_DIR/Transports
    INSTALLS += target
}
win32 {
    LIBS += -lcurl.dll
}


