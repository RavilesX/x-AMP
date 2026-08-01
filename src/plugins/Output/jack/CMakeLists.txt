project(libjack)

include(CheckCXXSourceCompiles)

# libjack and libsoxr
pkg_check_modules(JACK jack>=1.9.8 soxr IMPORTED_TARGET)

if(NOT JACK_FOUND)
    pkg_check_modules(JACK jack>=0.122.0 soxr IMPORTED_TARGET)
endif(NOT JACK_FOUND)

set(libjack_SRCS
  outputjackfactory.cpp
  outputjack.cpp
  bio2jack.c
  translations/translations.qrc
)

if(JACK_FOUND)
    add_library(jack MODULE ${libjack_SRCS})
    set_target_properties(jack PROPERTIES
                               COMPILE_FLAGS -pthread
                               LINK_FLAGS -pthread
    )
    target_link_libraries(jack PRIVATE Qt6::Widgets libqmmp PkgConfig::JACK)
    install(TARGETS jack DESTINATION ${PLUGIN_DIR}/Output)
endif(JACK_FOUND)

