project(libopus)

# libopus and libopusfile
pkg_check_modules(OPUS opus>=1.0.2 opusfile>=0.2 IMPORTED_TARGET)

set(libopus_SRCS
  decoder_opus.cpp
  decoderopusfactory.cpp
  opusmetadatamodel.cpp
  translations/translations.qrc
)

if(OPUS_FOUND)
    add_library(opus MODULE ${libopus_SRCS})
    target_link_libraries(opus PRIVATE Qt6::Widgets libqmmp PkgConfig::OPUS PkgConfig::TAGLIB)
    install(TARGETS opus DESTINATION ${PLUGIN_DIR}/Input)
endif(OPUS_FOUND)
