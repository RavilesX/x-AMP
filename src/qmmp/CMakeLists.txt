project(libqmmp)

add_definitions(-DQMMP_LIBRARY)
set(libqmmp_DEFINES
    LIB_DIR="/${LIB_DIR}"
    QMMP_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
    APP_NAME_SUFFIX="${APP_NAME_SUFFIX}"
)

if(NOT QMMP_VERSION_STABLE) #add svn revision
    execute_process(COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/svn_revision.sh
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE SVN_REVISION
                   OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(SVN_REVISION)
        set(libqmmp_DEFINES ${libqmmp_DEFINES} SVN_REVISION="${SVN_REVISION}")
    endif(SVN_REVISION)
endif(NOT QMMP_VERSION_STABLE)

set(libqmmp_DEFINES ${libqmmp_DEFINES} QMMP_PLUGIN_DIR="${CMAKE_INSTALL_PREFIX}/${PLUGIN_DIR}")

set(QMMP_DEFAULT_OUTPUT "" CACHE STRING "default output plugin")
if(QMMP_DEFAULT_OUTPUT)
    SET(libqmmp_DEFINES ${libqmmp_DEFINES} QMMP_DEFAULT_OUTPUT="${QMMP_DEFAULT_OUTPUT}")
    MESSAGE(STATUS "Default output: ${QMMP_DEFAULT_OUTPUT}")
endif(QMMP_DEFAULT_OUTPUT)


set(libqmmp_SRCS
    visual.cpp
    recycler.cpp
    decoder.cpp
    output.cpp
    outputwriter.cpp
    trackinfo.cpp
    fft.c
    equ/iir.c
    equ/iir_cfs.c
    equ/iir_fpu.c
    soundcore.cpp
    effect.cpp
    qmmp.cpp
    statehandler.cpp
    volumehandler.cpp
    metadatamodel.cpp
    tagmodel.cpp
    qmmpaudioengine.cpp
    abstractengine.cpp
    audioparameters.cpp
    inputsource.cpp
    fileinputsource.cpp
    emptyinputsource.cpp
    metadatamanager.cpp
    replaygain.cpp
    qmmpsettings.cpp
    audioconverter.cpp
    eqsettings.cpp
    qmmpevents.cpp
    qmmpplugincache.cpp
    channelmap.cpp
    channelconverter.cpp
    volume.cpp
    dithering.cpp
    visualbuffer.cpp
    cueparser.cpp
    qmmptextcodec.cpp
)

SET(libqmmp_DEVEL_HDRS
    abstractengine.h
    audioconverter.h
    audioparameters.h
    buffer.h
    channelmap.h
    cueparser.h
    decoderfactory.h
    decoder.h
    effectfactory.h
    effect.h
    enginefactory.h
    eqsettings.h
    inputsourcefactory.h
    inputsource.h
    metadatamanager.h
    metadatamodel.h
    outputfactory.h
    output.h
    qmmp_export.h
    qmmp.h
    qmmpsettings.h
    qmmptextcodec.h
    soundcore.h
    statehandler.h
    tagmodel.h
    trackinfo.h
    visualfactory.h
    visual.h
    volume.h
    volumehandler.h
)

add_library(libqmmp SHARED ${libqmmp_SRCS})
target_link_libraries(libqmmp PRIVATE Qt6::Widgets)
set_target_properties(libqmmp PROPERTIES VERSION ${QMMP_VERSION} SOVERSION ${QMMP_SOVERSION} OUTPUT_NAME qmmp${APP_NAME_SUFFIX})
target_compile_definitions(libqmmp PRIVATE ${libqmmp_DEFINES})
install(TARGETS libqmmp LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

#freebsd support
if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    target_link_directories(libqmmp PRIVATE /usr/local/lib)
    target_link_libraries(libqmmp PRIVATE iconv)
endif()

if(APP_NAME_SUFFIX)
    install(FILES ${libqmmp_DEVEL_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qmmp${APP_NAME_SUFFIX}/qmmp)
    SET(QMMP_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/qmmp${APP_NAME_SUFFIX})
else()
    install(FILES ${libqmmp_DEVEL_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qmmp)
    SET(QMMP_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/qmmp.pc.in ${CMAKE_CURRENT_BINARY_DIR}/qmmp${APP_NAME_SUFFIX}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmmp${APP_NAME_SUFFIX}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
