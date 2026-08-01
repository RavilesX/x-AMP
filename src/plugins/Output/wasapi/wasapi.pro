include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Output/wasapi

HEADERS += outputwasapifactory.h \
           outputwasapi.h \
           wasapisettingsdialog.h

SOURCES += outputwasapifactory.cpp \
           outputwasapi.cpp \
           wasapisettingsdialog.cpp

RESOURCES = translations/translations.qrc

LIBS += -lstrmiids -ldmoguids -lmsdmo -lole32 -loleaut32 -luuid -lgdi32 -lksuser

target.path = $$PLUGIN_DIR/Output
INSTALLS += target

FORMS += \
    wasapisettingsdialog.ui
