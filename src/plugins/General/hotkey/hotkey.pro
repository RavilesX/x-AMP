include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/hotkey

HEADERS += hotkeyfactory.h \
           hotkeymanager.h \
           hotkeydialog.h \
           hotkeysettingsdialog.h

SOURCES += hotkeyfactory.cpp \
           hotkeydialog.cpp \
           hotkeymanager_x11.cpp \
           hotkeymanager_win.cpp \
           hotkeysettingsdialog.cpp

FORMS += \
         hotkeydialog.ui \
         hotkeysettingsdialog.ui

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/General
    INSTALLS += target
    PKGCONFIG += x11 xcb
}
