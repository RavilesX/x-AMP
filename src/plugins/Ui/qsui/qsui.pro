include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Ui/qsui

SOURCES += \
    qsuiactionmanager.cpp \
    qsuicolorscheme.cpp \
    qsuicolorschemewidget.cpp \
    qsuicoverwidget.cpp \
    qsuiequalizer.cpp \
    qsuihotkeyeditor.cpp \
    qsuikeyboardmanager.cpp \
    qsuilistwidget.cpp \
    qsuilistwidgetdrawer.cpp \
    qsuilogo.cpp \
    qsuimainwindow.cpp \
    qsuiplaylistbrowser.cpp \
    qsuiplaylistheader.cpp \
    qsuipopupsettings.cpp \
    qsuipopupwidget.cpp \
    qsuipositionslider.cpp \
    aboutqsuidialog.cpp \
    qsuifactory.cpp \
    qsuisettings.cpp \
    qsuishortcutitem.cpp \
    qsuitabwidget.cpp \
    qsuitabbar.cpp \
    filesystembrowser.cpp \
    elidinglabel.cpp \
    qsuitoolbareditor.cpp \
    qsuistatusbareditor.cpp \
    volumeslider.cpp \
    qsuiquicksearch.cpp \
    qsuivisualization.cpp \
    qsuiwaveformseekbar.cpp \
    qsuistatusbar.cpp \
    dockwidgetlist.cpp
HEADERS += \
    qsuiactionmanager.h \
    qsuicolorscheme.h \
    qsuicolorschemewidget.h \
    qsuicoverwidget.h \
    qsuiequalizer.h \
    qsuihotkeyeditor.h \
    qsuikeyboardmanager.h \
    qsuilistwidget.h \
    qsuilistwidgetdrawer.h \
    qsuilogo.h \
    qsuimainwindow.h \
    qsuiplaylistbrowser.h \
    qsuiplaylistheader.h \
    qsuipopupsettings.h \
    qsuipopupwidget.h \
    qsuipositionslider.h \
    aboutqsuidialog.h \
    qsuifactory.h \
    qsuisettings.h \
    qsuishortcutitem.h \
    qsuitabwidget.h \
    qsuitabbar.h \
    filesystembrowser.h \
    elidinglabel.h \
    qsuitoolbareditor.h \
    qsuistatusbareditor.h \
    volumeslider.h \
    qsuiquicksearch.h \
    qsuivisualization.h \
    qsuiwaveformseekbar.h \
    qsuistatusbar.h \
    dockwidgetlist.h

FORMS += \
    forms/qsuihotkeyeditor.ui \
    forms/qsuimainwindow.ui \
    forms/qsuipopupsettings.ui \
    forms/aboutqsuidialog.ui \
    forms/qsuisettings.ui \
    forms/qsuitoolbareditor.ui \
    forms/qsuistatusbareditor.ui

RESOURCES += translations/translations.qrc resources/qsui_resources.qrc txt/qsui_txt.qrc

LIBS += $$QMMPUI_LIB

unix {
  target.path = $$PLUGIN_DIR/Ui
  INSTALLS += target
}

win32 {
  INCLUDEPATH += ./
}
