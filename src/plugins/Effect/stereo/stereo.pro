include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Effect/stereo

HEADERS += stereoplugin.h \
           effectstereofactory.h \
           stereosettingsdialog.h

SOURCES += stereoplugin.cpp \
           effectstereofactory.cpp \
           stereosettingsdialog.cpp

FORMS += \
    stereosettingsdialog.ui

RESOURCES = translations/translations.qrc

unix {
    target.path = $$PLUGIN_DIR/Effect
    INSTALLS += target
}
