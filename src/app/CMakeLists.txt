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
set_target_properties(qmmp PROPERTIES OUTPUT_NAME qmmp${APP_NAME_SUFFIX})

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
    install(FILES qmmp-opencda${APP_NAME_SUFFIX}.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/solid/actions)
    install(FILES qmmp${APP_NAME_SUFFIX}.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES images/16x16/qmmp${APP_NAME_SUFFIX}.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/16x16/apps)
    install(FILES images/32x32/qmmp${APP_NAME_SUFFIX}.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
    install(FILES images/48x48/qmmp${APP_NAME_SUFFIX}.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps)
    install(FILES images/56x56/qmmp${APP_NAME_SUFFIX}.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/56x56/apps)
    install(FILES images/64x64/qmmp${APP_NAME_SUFFIX}.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps)
    install(FILES images/128x128/qmmp${APP_NAME_SUFFIX}.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps)
    install(FILES images/256x256/qmmp${APP_NAME_SUFFIX}.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
    install(FILES images/scalable/qmmp${APP_NAME_SUFFIX}.svgz images/scalable/qmmp-simple${APP_NAME_SUFFIX}.svgz DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    install(FILES metainfo/com.ylsoftware.qmmp${APP_NAME_SUFFIX}.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif(UNIX)

if(USE_DIR_ASSOC)
    install(FILES qmmp-enqueue${APP_NAME_SUFFIX}.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES qmmp-dir${APP_NAME_SUFFIX}.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif(USE_DIR_ASSOC)
