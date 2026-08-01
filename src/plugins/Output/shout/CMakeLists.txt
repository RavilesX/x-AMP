project(libshout)

# libshout and other required libraries
pkg_check_modules(SHOUT ogg vorbis vorbisenc shout>=2.4.3 soxr IMPORTED_TARGET)

SET(libshout_SRCS
  shoutoutput.cpp
  outputshoutfactory.cpp
  shoutclient.cpp
  shoutsettingsdialog.cpp
  shoutsettingsdialog.ui
  translations/translations.qrc
)

if(SHOUT_FOUND)
    add_library(shout MODULE ${libshout_SRCS})
    target_link_libraries(shout PRIVATE Qt6::Widgets libqmmp PkgConfig::SHOUT)
    install(TARGETS shout DESTINATION ${PLUGIN_DIR}/Output)
endif(SHOUT_FOUND)
