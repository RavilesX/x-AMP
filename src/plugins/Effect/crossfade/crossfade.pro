include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Effect/crossfade

HEADERS += crossfadeplugin.h \
           crossfadesettingsdialog.h \
           effectcrossfadefactory.h

SOURCES += crossfadeplugin.cpp \
           crossfadesettingsdialog.cpp \
           effectcrossfadefactory.cpp

FORMS += \
    crossfadesettingsdialog.ui

RESOURCES = translations/translations.qrc

unix {
    target.path = $$PLUGIN_DIR/Effect
    INSTALLS += target
}
