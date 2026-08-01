include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Visual/analyzer

HEADERS += analyzer.h \
           analyzercolorwidget.h \
           analyzersettingsdialog.h \
           visualanalyzerfactory.h
SOURCES += analyzer.cpp \
           analyzercolorwidget.cpp \
           analyzersettingsdialog.cpp \
           visualanalyzerfactory.cpp

FORMS += \
    analyzersettingsdialog.ui

RESOURCES = translations/translations.qrc

unix{
   target.path = $$PLUGIN_DIR/Visual
   INSTALLS += target
}

INCLUDEPATH += ./
