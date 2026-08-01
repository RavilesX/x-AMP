include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Effect/bs2b

HEADERS += bs2bplugin.h \
           bs2bsettingsdialog.h \
           effectbs2bfactory.h

SOURCES += bs2bplugin.cpp \
           bs2bsettingsdialog.cpp \
           effectbs2bfactory.cpp

FORMS += \
    bs2bsettingsdialog.ui

RESOURCES = translations/translations.qrc

unix {
    target.path = $$PLUGIN_DIR/Effect
    INSTALLS += target
    PKGCONFIG += libbs2b
    LIBS += -L/usr/lib -I/usr/include
}

win32 {
    LIBS += -lbs2b
}
