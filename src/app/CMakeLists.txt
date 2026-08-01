project(app)

set(app_SRCS
    builtincommandlineoption.cpp
    main.cpp
    qmmpstarter.cpp
    images/images.qrc
    translations/qmmp_locales.qrc
)

add_executable(qmmp ${app_SRCS})
target_link_libraries(qmmp PRIVATE Qt6::Widgets Qt6::Network libqmmp libqmmpui)
set_target_properties(qmmp PROPERTIES OUTPUT_NAME ${APP_BINARY_NAME})

if(WIN32)
    target_sources(qmmp PRIVATE qmmp.rc)
    target_compile_definitions(qmmp PRIVATE QMMP_NO_CLI)
    set_target_properties(qmmp PROPERTIES WIN32_EXECUTABLE TRUE)    
    #cli version
    add_executable(qmmp-cli ${app_SRCS})
    target_link_libraries(qmmp-cli PRIVATE Qt6::Widgets Qt6::Network libqmmp libqmmpui)
    install(TARGETS qmmp-cli DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(WIN32)

install(TARGETS qmmp DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX)
    #x-AMP: source names stay as upstream has them (images.qrc and main.cpp
    #reference them); the fork's name is applied on install via RENAME
    install(FILES qmmp-opencda.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/solid/actions
            RENAME ${APP_BINARY_NAME}-opencda.desktop)
    install(FILES qmmp.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
            RENAME ${APP_BINARY_NAME}.desktop)
    foreach(size 16x16 32x32 48x48 56x56 64x64 128x128 256x256)
        install(FILES images/${size}/qmmp.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}/apps
                RENAME ${APP_BINARY_NAME}.png)
    endforeach()
    install(FILES images/scalable/qmmp.svgz DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
            RENAME ${APP_BINARY_NAME}.svgz)
    install(FILES images/scalable/qmmp-simple.svgz DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
            RENAME ${APP_BINARY_NAME}-simple.svgz)
    #AppStream requires the file name to match the component id, which is the
    #fork's own rather than a suffixed upstream one
    install(FILES metainfo/io.github.ravilesx.xamp.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif(UNIX)

if(USE_DIR_ASSOC)
    install(FILES qmmp-enqueue.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
            RENAME ${APP_BINARY_NAME}-enqueue.desktop)
    install(FILES qmmp-dir.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
            RENAME ${APP_BINARY_NAME}-dir.desktop)
endif(USE_DIR_ASSOC)
