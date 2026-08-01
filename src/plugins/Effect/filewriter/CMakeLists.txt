project(libfilewriter)

pkg_check_modules(VORBISENC vorbis ogg vorbisenc IMPORTED_TARGET)

set(libfilewriter_SRCS
    filewriterplugin.cpp
    filewritersettingsdialog.cpp
    effectfilewriterfactory.cpp
    filewritersettingsdialog.ui
    translations/translations.qrc
)

if(VORBISENC_FOUND)
    add_library(filewriter MODULE ${libfilewriter_SRCS})
    target_link_libraries(filewriter PRIVATE Qt6::Widgets libqmmp libqmmpui PkgConfig::VORBISENC)
    install(TARGETS filewriter DESTINATION ${PLUGIN_DIR}/Effect)
endif(VORBISENC_FOUND)
