include(../../qmmp.pri)

VERSION = $$QMMP_VERSION
INCLUDEPATH += ../
TEMPLATE = lib
DEFINES += QMMPUI_LIBRARY
QT += network

CONFIG += warn_on \
    shared \
    qt \
    thread

QMAKE_LIBDIR += ../../lib \
    qmmpui

unix {
    TARGET = ../../lib/qmmpui$$APP_NAME_SUFFIX
    LIBS += -L../../lib -lqmmp$$APP_NAME_SUFFIX
    target.path = $$LIB_DIR
}

win32 {
    TARGET = ../../../bin/qmmpui
    LIBS += -L../../bin -lqmmp2 -lole32
    HEADERS +=
    SOURCES +=
    FORMS +=
}

HEADERS += general.h \
    colorwidget.h \
    coverloader_p.h \
    editorbase_p.h \
    generalfactory.h \
    playlistformat.h \
    playlistparser.h \
    commandlinemanager.h \
    filedialog.h \
    filedialogfactory.h \
    qtfiledialog_p.h \
    playlistitem.h \
    playlistmodel.h \
    playstate_p.h \
    fileloader_p.h \
    mediaplayer.h \
    detailsdialog.h \
    shortcutdialog.h \
    tageditor_p.h \
    playlistmanager.h \
    templateeditor.h \
    uifactory.h \
    uiloader.h \
    uihelper.h \
    jumptotrackdialog_p.h \
    configdialog.h \
    pluginitem_p.h \
    aboutdialog_p.h \
    qmmpuisettings.h \
    radioitemdelegate_p.h \
    playlistdownloader.h \
    addurldialog_p.h \
    qmmpuiplugincache_p.h \
    playlistgroup.h \
    playlistgroup_p.h \
    playlisttrack.h \
    playlisttrack_p.h \
    playlistcontainer_p.h \
    groupedcontainer_p.h \
    normalcontainer_p.h \
    playlisttask_p.h \
    metadataformatter.h \
    columneditor_p.h \
    playlistheadermodel.h \
    metadatahelper_p.h \
    coverviewer_p.h \
    metadataformattermenu.h \
    qmmpui_export.h \
    covereditor_p.h \
    commandlinehandler.h \
    cueeditor_p.h \
    lyricseditor_p.h \
    visualmenu.h

SOURCES += general.cpp \
    colorwidget.cpp \
    coverloader.cpp \
    editorbase.cpp \
    playlistparser.cpp \
    commandlinemanager.cpp \
    filedialog.cpp \
    qtfiledialog.cpp \
    playlistmodel.cpp \
    playstate.cpp \
    playlistitem.cpp \
    fileloader.cpp \
    mediaplayer.cpp \
    detailsdialog.cpp \
    playlistmanager.cpp \
    shortcutdialog.cpp \
    templateeditor.cpp \
    uiloader.cpp \
    uihelper.cpp \
    jumptotrackdialog.cpp \
    configdialog.cpp \
    pluginitem.cpp \
    aboutdialog.cpp \
    qmmpuisettings.cpp \
    radioitemdelegate.cpp \
    playlistdownloader.cpp \
    addurldialog.cpp \
    qmmpuiplugincache.cpp \
    playlistgroup.cpp \
    playlisttrack.cpp \
    groupedcontainer.cpp \
    normalcontainer.cpp \
    playlistcontainer.cpp \
    playlisttask.cpp \
    metadataformatter.cpp \
    columneditor.cpp \
    playlistheadermodel.cpp \
    metadatahelper.cpp \
    coverviewer.cpp \
    metadataformattermenu.cpp \
    covereditor.cpp \
    commandlinehandler.cpp \
    generalfactory.cpp \
    cueeditor.cpp \
    lyricseditor.cpp \
    tageditor.cpp \
    visualmenu.cpp

FORMS += forms/detailsdialog.ui \
    forms/shortcutdialog.ui \
    forms/tageditor.ui \
    forms/templateeditor.ui \
    forms/jumptotrackdialog.ui \
    forms/configdialog.ui \
    forms/aboutdialog.ui \
    forms/addurldialog.ui \
    forms/columneditor.ui \
    forms/covereditor.ui \
    forms/texteditor.ui

unix:DESTDIR = .
RESOURCES += translations/libqmmpui_locales.qrc \
    images/qmmpui_images.qrc \
    txt/txt.qrc
TRANSLATIONS = translations/libqmmpui_ru.ts \
    translations/libqmmpui_tr.ts \
    translations/libqmmpui_zh_CN.ts \
    translations/libqmmpui_cs.ts \
    translations/libqmmpui_pt_BR.ts \
    translations/libqmmpui_uk_UA.ts \
    translations/libqmmpui_zh_TW.ts \
    translations/libqmmpui_de.ts \
    translations/libqmmpui_it.ts \
    translations/libqmmpui_lt.ts \
    translations/libqmmpui_pl_PL.ts \
    translations/libqmmpui_nl.ts \
    translations/libqmmpui_ja.ts \
    translations/libqmmpui_es.ts \
    translations/libqmmpui_sk.ts \
    translations/libqmmpui_sr_BA.ts \
    translations/libqmmpui_sr_RS.ts
unix {
    devel.files += \
    colorwidget.h \
    commandlinehandler.h \
    commandlinemanager.h \
    configdialog.h \
    detailsdialog.h \
    filedialogfactory.h \
    filedialog.h \
    generalfactory.h \
    general.h \
    mediaplayer.h \
    metadataformatter.h \
    metadataformattermenu.h \
    playlistdownloader.h \
    playlistformat.h \
    playlistgroup.h \
    playlistheadermodel.h \
    playlistitem.h \
    playlistmanager.h \
    playlistmodel.h \
    playlistparser.h \
    playlisttrack.h \
    qmmpui_export.h \
    qmmpuisettings.h \
    shortcutdialog.h \
    templateeditor.h \
    uifactory.h \
    uihelper.h \
    uiloader.h \
    visualmenu.h

    isEmpty(APP_NAME_SUFFIX) {
        devel.path = $$PREFIX/include/qmmpui
    } else {
        devel.path = $$PREFIX/include/qmmp$${APP_NAME_SUFFIX}/qmmpui
    }
    INSTALLS += target \
        devel
}

unix {
    CONFIG += create_pc create_prl no_install_prl
    QMAKE_PKGCONFIG_NAME = qmmpui$${APP_NAME_SUFFIX}
    QMAKE_PKGCONFIG_DESCRIPTION = qmmp user interface library
    QMAKE_PKGCONFIG_REQUIRES = Qt6Core Qt6Gui Qt6Widgets Qt6Network qmmp$${APP_NAME_SUFFIX}
    QMAKE_PKGCONFIG_DESTDIR = pkgconfig
    QMAKE_PKGCONFIG_PREFIX = $$PREFIX
    QMAKE_PKGCONFIG_LIBDIR = $$target.path
    isEmpty(APP_NAME_SUFFIX) {
        QMAKE_PKGCONFIG_INCDIR = $$PREFIX/include
    } else {
        QMAKE_PKGCONFIG_INCDIR = $$PREFIX/include/qmmp$${APP_NAME_SUFFIX}
    }
}
