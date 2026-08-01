project(libvorbis)

# libvorbis
pkg_check_modules(VORBIS ogg vorbis vorbisfile IMPORTED_TARGET)

SET(libvorbis_SRCS
  decoder_vorbis.cpp
  decodervorbisfactory.cpp
  vorbismetadatamodel.cpp
  translations/translations.qrc
)

if(VORBIS_FOUND)
    add_library(vorbis MODULE ${libvorbis_SRCS})
    target_link_libraries(vorbis PRIVATE Qt6::Widgets libqmmp PkgConfig::VORBIS PkgConfig::TAGLIB)
    install(TARGETS vorbis DESTINATION ${PLUGIN_DIR}/Input)
endif(VORBIS_FOUND)
