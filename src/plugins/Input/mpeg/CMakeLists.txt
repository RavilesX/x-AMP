project(libmpeg)

include(CheckIncludeFileCXX)

# libmad
if(USE_MAD)
    pkg_search_module(MAD libmad mad IMPORTED_TARGET)
endif(USE_MAD)

# libmpg123
if(USE_MPG123)
    pkg_search_module(MPG123 libmpg123>=1.13.0 IMPORTED_TARGET)
endif(USE_MPG123)

option(USE_LIBRCD "charset detection for ID3v1/2 tags using librcd" OFF)

if(USE_LIBRCD)
    pkg_search_module(LIBRCD librcd IMPORTED_TARGET)
endif(USE_LIBRCD)

if(LIBRCD_FOUND)
    add_definitions(-DWITH_LIBRCD)
endif(LIBRCD_FOUND)


set(libmpeg_SRCS
  decodermpegfactory.cpp
  mpegsettingsdialog.cpp
  tagextractor.cpp
  mpegmetadatamodel.cpp
  mpegsettingsdialog.ui
  translations/translations.qrc
)

if(MAD_FOUND)
    add_definitions(-DWITH_MAD)
    set(libmpeg_SRCS ${libmpeg_SRCS} decoder_mad.cpp)
endif(MAD_FOUND)

if(MPG123_FOUND)
    add_definitions(-DWITH_MPG123)
    set(libmpeg_SRCS ${libmpeg_SRCS} decoder_mpg123.cpp)
endif(MPG123_FOUND)

if(MAD_FOUND OR MPG123_FOUND)
    add_library(mpeg MODULE ${libmpeg_SRCS})
    target_link_libraries(mpeg PRIVATE Qt6::Widgets libqmmp PkgConfig::TAGLIB)
    if(MAD_FOUND)
        target_link_libraries(mpeg PRIVATE PkgConfig::MAD)
    endif()
    if(MPG123_FOUND)
        target_link_libraries(mpeg PRIVATE PkgConfig::MPG123)
    endif()
    if(LIBRCD_FOUND)
        target_link_libraries(mpeg PRIVATE PkgConfig::LIBRCD)
    endif()
    install(TARGETS mpeg DESTINATION ${PLUGIN_DIR}/Input)
endif(MAD_FOUND OR MPG123_FOUND)
