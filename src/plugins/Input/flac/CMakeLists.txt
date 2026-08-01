project(libflac)

# libflac
pkg_search_module(FLAC flac IMPORTED_TARGET)

SET(libflac_SRCS
  decoder_flac.cpp
  decoderflacfactory.cpp
  flacmetadatamodel.cpp
  translations/translations.qrc
)

if(FLAC_FOUND)
    add_library(flac MODULE ${libflac_SRCS})
    target_link_libraries(flac PRIVATE Qt6::Widgets libqmmp PkgConfig::FLAC PkgConfig::TAGLIB)
    install(TARGETS flac DESTINATION ${PLUGIN_DIR}/Input)
endif(FLAC_FOUND)
