project(libqmmpui)

add_definitions(-DQMMPUI_LIBRARY)

set(QMMP_DEFAULT_UI "" CACHE STRING "default user interface")
if(QMMP_DEFAULT_UI)
    #x-AMP: was QMMP_DEFAULT_OUTPUT here, so -DQMMP_DEFAULT_UI=xui compiled in
    #the name of an output plugin and the interface it named was never found
    set(libqmmpui_DEFINES ${libqmmpui_DEFINES} QMMP_DEFAULT_UI="${QMMP_DEFAULT_UI}")
    message(STATUS "Default user interface: ${QMMP_DEFAULT_UI}")
endif(QMMP_DEFAULT_UI)


set(libqmmpui_SRCS
    general.cpp
    uihelper.cpp
    playlistparser.cpp
    commandlinemanager.cpp
    commandlinehandler.cpp
    filedialog.cpp
    qtfiledialog.cpp
    fileloader.cpp
    playstate.cpp
    playlistmodel.cpp
    playlistitem.cpp
    playlistgroup.cpp
    playlisttrack.cpp
    playlistcontainer.cpp
    playlistqueue.cpp
    normalcontainer.cpp
    groupedcontainer.cpp
    mediaplayer.cpp
    detailsdialog.cpp
    tageditor.cpp
    playlistmanager.cpp
    metadataformatter.cpp
    metadataformattermenu.cpp
    templateeditor.cpp
    uiloader.cpp
    jumptotrackdialog.cpp
    configdialog.cpp
    pluginitem.cpp
    aboutdialog.cpp
    qmmpuisettings.cpp
    radioitemdelegate.cpp
    playlistdownloader.cpp
    addurldialog.cpp
    qmmpuiplugincache.cpp
    playlisttask.cpp
    playlistheadermodel.cpp
    columneditor.cpp
    metadatahelper.cpp
    coverviewer.cpp
    covereditor.cpp
    coverloader.cpp
    generalfactory.cpp
    cueeditor.cpp
    lyricseditor.cpp
    visualmenu.cpp
    colorwidget.cpp
    scheduler.cpp
    shortcutdialog.cpp
    editorbase.cpp
    forms/detailsdialog.ui
    forms/tageditor.ui
    forms/templateeditor.ui
    forms/jumptotrackdialog.ui
    forms/configdialog.ui
    forms/aboutdialog.ui
    forms/addurldialog.ui
    forms/columneditor.ui
    forms/covereditor.ui
    forms/texteditor.ui
    forms/shortcutdialog.ui
    translations/libqmmpui_locales.qrc
    txt/txt.qrc
    images/qmmpui_images.qrc
)

SET(libqmmpui_DEVEL_HDRS
    colorwidget.h
    commandlinehandler.h
    commandlinemanager.h
    configdialog.h
    detailsdialog.h
    filedialogfactory.h
    filedialog.h
    generalfactory.h
    general.h
    mediaplayer.h
    metadataformatter.h
    metadataformattermenu.h
    playlistdownloader.h
    playlistformat.h
    playlistgroup.h
    playlistheadermodel.h
    playlistitem.h
    playlistmanager.h
    playlistmodel.h
    playlistparser.h
    playlisttrack.h
    qmmpui_export.h
    qmmpuisettings.h
    scheduler.h
    shortcutdialog.h
    templateeditor.h
    uifactory.h
    uihelper.h
    uiloader.h
    visualmenu.h
)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${CMAKE_CURRENT_SOURCE_DIR}/forms)

add_library(libqmmpui SHARED ${libqmmpui_SRCS})
target_link_libraries(libqmmpui PRIVATE Qt6::Widgets Qt6::Network libqmmp)
#x-AMP: the scheduler asks logind to suspend or power off the machine; without
#D-Bus it falls back to systemctl
if(Qt6DBus_FOUND)
    target_link_libraries(libqmmpui PRIVATE Qt6::DBus)
    set(libqmmpui_DEFINES ${libqmmpui_DEFINES} QMMPUI_HAS_DBUS)
endif(Qt6DBus_FOUND)
set_target_properties(libqmmpui PROPERTIES VERSION ${QMMP_VERSION} SOVERSION ${QMMP_SOVERSION} OUTPUT_NAME qmmpui${APP_NAME_SUFFIX})
target_compile_definitions(libqmmpui PRIVATE ${libqmmpui_DEFINES})
install(TARGETS libqmmpui LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})


if(APP_NAME_SUFFIX)
    install(FILES ${libqmmpui_DEVEL_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qmmp${APP_NAME_SUFFIX}/qmmpui)
    set(QMMP_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/qmmp${APP_NAME_SUFFIX})
else()
    install(FILES ${libqmmpui_DEVEL_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qmmpui)
    set(QMMP_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/qmmpui.pc.in ${CMAKE_CURRENT_BINARY_DIR}/qmmpui${APP_NAME_SUFFIX}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmmpui${APP_NAME_SUFFIX}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
