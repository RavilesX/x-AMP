project(libarchive)

# libarchive
pkg_search_module(LIBARCHIVE libarchive>=3.2.0 IMPORTED_TARGET)

set(libarchive_SRCS
  archiveinputdevice.cpp
  decoder_archive.cpp
  archivetagreader.cpp
  decoderarchivefactory.cpp
  translations/translations.qrc
)

if(LIBARCHIVE_FOUND)
    add_library(archive MODULE ${libarchive_SRCS})
    target_link_libraries(archive PRIVATE Qt6::Widgets libqmmp PkgConfig::LIBARCHIVE PkgConfig::TAGLIB)
    install(TARGETS archive DESTINATION ${PLUGIN_DIR}/Input)
endif(LIBARCHIVE_FOUND)
