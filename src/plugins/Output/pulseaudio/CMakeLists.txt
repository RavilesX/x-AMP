project(libpulseaudio)

# pulseaudio
pkg_check_modules(PULSE libpulse>=0.9.19 IMPORTED_TARGET)

set(libpulseaudio_SRCS
  outputpulseaudio.cpp
  outputpulseaudiofactory.cpp
  translations/translations.qrc
)

if(PULSE_FOUND)
    add_library(pulseaudio MODULE ${libpulseaudio_SRCS})
    target_link_libraries(pulseaudio PRIVATE Qt6::Widgets libqmmp PkgConfig::PULSE)
    install(TARGETS pulseaudio DESTINATION ${PLUGIN_DIR}/Output)
endif(PULSE_FOUND)

