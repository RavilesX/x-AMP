project(qsui)

set(libqsui_SRCS
    aboutqsuidialog.cpp
    qsuiactionmanager.cpp
    dockwidgetlist.cpp
    elidinglabel.cpp
    qsuiequalizer.cpp
    filesystembrowser.cpp
    qsuikeyboardmanager.cpp
    qsuiplaylistbrowser.cpp
    qsuicoverwidget.cpp
    qsuifactory.cpp
    qsuihotkeyeditor.cpp
    qsuilistwidget.cpp
    qsuilistwidgetdrawer.cpp
    qsuilogo.cpp
    qsuimainwindow.cpp
    qsuiplaylistheader.cpp
    qsuipopupsettings.cpp
    qsuipopupwidget.cpp
    qsuipositionslider.cpp
    qsuiquicksearch.cpp
    qsuisettings.cpp
    qsuistatusbar.cpp
    qsuitabbar.cpp
    qsuitabwidget.cpp
    qsuivisualization.cpp
    qsuiwaveformseekbar.cpp
    qsuishortcutitem.cpp
    qsuitoolbareditor.cpp
    qsuistatusbareditor.cpp
    qsuicolorscheme.cpp
    qsuicolorschemewidget.cpp
    volumeslider.cpp
    forms/qsuimainwindow.ui
    forms/qsuipopupsettings.ui
    forms/qsuisettings.ui
    forms/aboutqsuidialog.ui
    forms/qsuitoolbareditor.ui
    forms/qsuistatusbareditor.ui
    forms/qsuihotkeyeditor.ui
    translations/translations.qrc
    txt/qsui_txt.qrc
    resources/qsui_resources.qrc
)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${CMAKE_CURRENT_SOURCE_DIR}/forms)

add_library(qsui MODULE ${libqsui_SRCS})
target_link_libraries(qsui PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS qsui DESTINATION ${PLUGIN_DIR}/Ui)
