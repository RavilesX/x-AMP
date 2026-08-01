project(libsndfile)

# libsndfile
pkg_search_module(SNDFILE sndfile>=1.0.22 IMPORTED_TARGET)

SET(libsndfile_SRCS
  decoder_sndfile.cpp
  decodersndfilefactory.cpp
  sndfilemetadatamodel.cpp
  translations/translations.qrc
)

if(SNDFILE_FOUND)
    add_library(sndfile MODULE ${libsndfile_SRCS})
    target_link_libraries(sndfile PRIVATE Qt6::Widgets libqmmp PkgConfig::SNDFILE PkgConfig::TAGLIB)
    install(TARGETS sndfile DESTINATION ${PLUGIN_DIR}/Input)
endif(SNDFILE_FOUND)



