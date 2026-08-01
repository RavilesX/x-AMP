include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Effect/soxr

HEADERS += soxresampler.h \
 effectsoxrfactory.h \
 soxrsettingsdialog.h

SOURCES += soxresampler.cpp \
 effectsoxrfactory.cpp \
 soxrsettingsdialog.cpp

FORMS += \
    soxrsettingsdialog.ui

RESOURCES = translations/translations.qrc

unix {
    target.path = $$PLUGIN_DIR/Effect
    INSTALLS += target
    PKGCONFIG += soxr
    LIBS += -L/usr/lib -I/usr/include
}

win32 {
    LIBS += -lsoxr
}
