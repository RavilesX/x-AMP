include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/statusicon

HEADERS += statusiconfactory.h \
    statusicon.h \
    qmmptrayicon.h \
    statusiconcoverwidget.h \
    statusiconsettingsdialog.h

SOURCES += statusiconfactory.cpp \
    statusicon.cpp \
    qmmptrayicon.cpp \
    statusiconcoverwidget.cpp \
    statusiconsettingsdialog.cpp

contains(DEFINES, QMMP_WS_X11) {
    HEADERS += statusiconpopupwidget.h
    SOURCES += statusiconpopupwidget.cpp
}

FORMS += \
    statusiconsettingsdialog.ui

RESOURCES = translations/translations.qrc \
            images/tray_images.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/General
    INSTALLS += target
}
