project(libprojectm)

# projectM
if(QT_FEATURE_opengl)
    message(STATUS "Found desktop OpenGL implementation")
    # projectM 4.0
    find_package(projectM4 COMPONENTS Playlist)
    if(projectM4_FOUND)
        add_definitions(-DPROJECTM_4)
        set(PROJECTM_FOUND TRUE CACHE INTERNAL "projectm")
        set(PROJECTM_PRESET_PATH ${CMAKE_INSTALL_DATADIR}/presets CACHE STRING "projectM preset search path")
        set(PROJECTM_TEXTURE_PATH ${CMAKE_INSTALL_DATADIR}/textures CACHE STRING "projectM texture search path")
        set(libprojectm_DEFINES ${libprojectm_DEFINES}
           PROJECTM_PRESET_PATH=\"${PROJECTM_PRESET_PATH}\"
           PROJECTM_TEXTURE_PATH=\"${PROJECTM_TEXTURE_PATH}\"
        )
        message(STATUS "Found projectM 4.x")
    else(projectM4_FOUND)
        # projectM 2.x/3.x 
        pkg_search_module(PROJECTM libprojectM>=2.0.0 IMPORTED_TARGET)
        if(PROJECTM_FOUND)
           find_file(PROJECTM_CONFIG config.inp
                     PATHS "/usr/share/projectM/" "/usr/local/share/projectM/")
           if(PROJECTM_VERSION VERSION_GREATER_EQUAL 3.1.0)
               ADD_DEFINITIONS(-DPROJECTM_31)
               MESSAGE(STATUS "Found projectM 3.x")
           else()
               MESSAGE(STATUS "Found projectM 2.x")
           endif()
        endif(PROJECTM_FOUND)
     endif(projectM4_FOUND)
else(QT_FEATURE_opengl)
    message(WARNING "Unsuppoted OpenGL implementation, projectM disabled")
endif(QT_FEATURE_opengl)

if(PROJECTM_CONFIG)
    set(libprojectm_DEFINES ${libprojectm_DEFINES} PROJECTM_CONFIG=\"${PROJECTM_CONFIG}\")
    message(STATUS "Found projectM configuration: ${PROJECTM_CONFIG}")
endif(PROJECTM_CONFIG)

set(libprojectm_SRCS
    projectmplugin.cpp
    visualprojectmfactory.cpp
    translations/translations.qrc
)

if(projectM4_FOUND)
    set(libprojectm_SRCS ${libprojectm_SRCS} projectm4widget.cpp)
else(projectM4_FOUND)
    set(libprojectm_SRCS ${libprojectm_SRCS} projectmwidget.cpp projectmwrapper.cpp)
endif(projectM4_FOUND)

if(PROJECTM_FOUND)
    add_library(projectm MODULE ${libprojectm_SRCS})

    if(projectM4_FOUND)
        target_link_libraries(projectm PRIVATE Qt6::OpenGLWidgets libqmmp libprojectM::playlist)
    else(projectM4_FOUND)
        target_compile_definitions(projectm PRIVATE ${libprojectm_DEFINES})
        target_link_libraries(projectm PRIVATE Qt6::OpenGLWidgets libqmmp PkgConfig::PROJECTM)
        if(WIN32)
            target_link_libraries(projectm PRIVATE -lopengl32)
        else(WIN32)
            target_link_libraries(projectm PRIVATE -lGL)
        endif(WIN32)
    endif(projectM4_FOUND)
    install(TARGETS projectm DESTINATION ${PLUGIN_DIR}/Visual)
endif(PROJECTM_FOUND)
