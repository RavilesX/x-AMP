include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/General/library

QT += sql

HEADERS += libraryfactory.h \
    library.h \
    librarysettingsdialog.h \
    librarywidget.h \
    librarymodel.h

SOURCES += libraryfactory.cpp \
    library.cpp \
    librarysettingsdialog.cpp \
    librarywidget.cpp \
    librarymodel.cpp


RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/General
    INSTALLS += target
}

FORMS += \
    librarysettingsdialog.ui \
    librarywidget.ui
