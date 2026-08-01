include(../../plugins.pri)

QT += openglwidgets

TARGET = $$PLUGINS_PREFIX/Visual/projectm

HEADERS += visualprojectmfactory.h \
           projectmplugin.h

SOURCES += visualprojectmfactory.cpp \
           projectmplugin.cpp

RESOURCES = translations/translations.qrc

#DEFINES += PROJECTM_31
#DEFINES += PROJECTM_4

freebsd:DEFINES += PROJECTM_31

contains(DEFINES, PROJECTM_4) {
    HEADERS += projectm4widget.h

    SOURCES += projectm4widget.cpp
} else {
    HEADERS += projectmwidget.h \
               projectmwrapper.h

    SOURCES += projectmwidget.cpp \
               projectmwrapper.cpp
}

unix {
    target.path = $$PLUGIN_DIR/Visual
    INSTALLS += target
    LIBS += -L/usr/lib -I/usr/include

    contains(DEFINES, PROJECTM_4) {
        INCLUDEPATH += /home/user/projectm/include
        QMAKE_LIBDIR += /home/user/projectm/lib
        LIBS += -lprojectM-4 -lprojectM-4-playlist
    } else {
        PKGCONFIG += libprojectM
        #projectM config path
        PROJECTM_CONFIG_FILES = /usr/share/projectM/config.inp \
                                /usr/local/share/projectM/config.inp
        for(path, PROJECTM_CONFIG_FILES) {
            exists($$path) {
                message("found projectm configuration: "$$path)
                DEFINES += PROJECTM_CONFIG=\\\"$$path\\\"
            }
        }
    }
}

win32 {
    LIBS += -lprojectM.dll -lopengl32
}
