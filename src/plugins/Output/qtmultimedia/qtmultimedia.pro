include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Output/qtmultimedia

QT += multimedia

FORMS += \
    qtmultimediasettingsdialog.ui

HEADERS += outputqtmultimediafactory.h \
           outputqtmultimedia.h \
           qtmultimediasettingsdialog.h

SOURCES += outputqtmultimediafactory.cpp \
           outputqtmultimedia.cpp \
           qtmultimediasettingsdialog.cpp

RESOURCES = translations/translations.qrc

unix {
    target.path = $$PLUGIN_DIR/Output
    INSTALLS += target
}
