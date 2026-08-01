include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Ui/skinned

HEADERS += \
    skin.h \
    skinnedactionmanager.h \
    skinnedbalancebar.h \
    skinnedbutton.h \
    skinneddisplay.h \
    skinnedeqgraph.h \
    skinnedeqslider.h \
    skinnedeqtitlebar.h \
    skinnedeqwidget.h \
    skinnedhorizontalslider.h \
    skinnedhotkeyeditor.h \
    skinnedkeyboardmanager.h \
    skinnedlistwidget.h \
    skinnedlistwidgetdrawer.h \
    skinnedmainwindow.h \
    skinnednumber.h \
    skinnedplaylist.h \
    skinnedplaylistbrowser.h \
    skinnedplaylistcontrol.h \
    skinnedplaylistheader.h \
    skinnedplaylistselector.h \
    skinnedplaylistslider.h \
    skinnedplaylisttitlebar.h \
    skinnedplaystatus.h \
    skinnedpopupsettings.h \
    skinnedpopupwidget.h \
    skinnedpositionbar.h \
    skinnedpreseteditor.h \
    skinnedshortcutitem.h \
    skinnedtextscroller.h \
    skinnedtimeindicator.h \
    skinnedtitlebar.h \
    skinnedtitlebarcontrol.h \
    skinnedtogglebutton.h \
    skinnedvisualization.h \
    skinnedvolumebar.h \
    pixmapwidget.h \
    dock.h \
    monostereo.h \
    symboldisplay.h \
    skinreader.h \
    shadedvisual.h \
    shadedbar.h \
    cursorimage.h \
    windowsystem.h \
    skinnedfactory.h \
    skinnedsettings.h

SOURCES += \
    skin.cpp \
    skinnedactionmanager.cpp \
    skinnedbalancebar.cpp \
    skinnedbutton.cpp \
    skinneddisplay.cpp \
    skinnedeqgraph.cpp \
    skinnedeqslider.cpp \
    skinnedeqtitlebar.cpp \
    skinnedeqwidget.cpp \
    skinnedhorizontalslider.cpp \
    skinnedhotkeyeditor.cpp \
    skinnedkeyboardmanager.cpp \
    skinnedlistwidget.cpp \
    skinnedlistwidgetdrawer.cpp \
    skinnedmainwindow.cpp \
    skinnednumber.cpp \
    skinnedplaylist.cpp \
    skinnedplaylistbrowser.cpp \
    skinnedplaylistcontrol.cpp \
    skinnedplaylistheader.cpp \
    skinnedplaylistselector.cpp \
    skinnedplaylistslider.cpp \
    skinnedplaylisttitlebar.cpp \
    skinnedplaystatus.cpp \
    skinnedpopupsettings.cpp \
    skinnedpopupwidget.cpp \
    skinnedpositionbar.cpp \
    skinnedpreseteditor.cpp \
    skinnedshortcutitem.cpp \
    skinnedtextscroller.cpp \
    skinnedtimeindicator.cpp \
    skinnedtitlebar.cpp \
    skinnedtitlebarcontrol.cpp \
    skinnedtogglebutton.cpp \
    skinnedvisualization.cpp \
    skinnedvolumebar.cpp \
    pixmapwidget.cpp \
    dock.cpp \
    monostereo.cpp \
    symboldisplay.cpp \
    skinreader.cpp \
    shadedvisual.cpp \
    shadedbar.cpp \
    cursorimage.cpp \
    windowsystem.cpp \
    skinnedfactory.cpp \
    skinnedsettings.cpp

FORMS += \
    forms/skinnedhotkeyeditor.ui \
    forms/skinnedplaylistbrowser.ui \
    forms/skinnedpopupsettings.ui \
    forms/skinnedpreseteditor.ui \
    forms/skinnedsettings.ui

RESOURCES = resources/resources.qrc \
            glare/glare.qrc \
            translations/translations.qrc

LIBS += $$QMMPUI_LIB

unix {
    target.path = $$PLUGIN_DIR/Ui
    scripts.files = scripts/kwin.sh scripts/kwin6.sh
    scripts.path = $$DATA_DIR/qmmp$${APP_NAME_SUFFIX}/scripts
    INSTALLS += target
    PKGCONFIG += x11
    DEFINES += QMMP_WS_X11
}
