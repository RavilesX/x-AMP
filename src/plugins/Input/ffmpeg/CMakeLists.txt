project(libffmpeg)

# fixes ffmpeg defines
add_definitions(-D__STDC_CONSTANT_MACROS)

# minimal supported versions
# ffmpeg-3.4: libavcodec-57.107.100, libavformat-57.83.100, libavutil-55.78.100
pkg_check_modules(FFMPEG libavcodec>=57.107.0 libavformat>=57.83.0 libavutil>=55.78.0 IMPORTED_TARGET)

SET(libffmpeg_SRCS
  decoder_ffmpeg.cpp
  decoder_ffmpegcue.cpp
  decoder_ffmpegm4b.cpp
  decoderffmpegfactory.cpp
  ffmpegmetadatamodel.cpp
  ffmpegsettingsdialog.cpp
  replaygainreader.cpp
  ffmpegsettingsdialog.ui
  translations/translations.qrc
)

if(FFMPEG_FOUND)
    add_library(ffmpeg MODULE ${libffmpeg_SRCS})
    target_link_libraries(ffmpeg PRIVATE Qt6::Widgets libqmmp PkgConfig::FFMPEG PkgConfig::TAGLIB)
    install(TARGETS ffmpeg DESTINATION ${PLUGIN_DIR}/Input)
endif(FFMPEG_FOUND)
