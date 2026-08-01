project(libscrobbler)

set(libscrobbler_SRCS
    scrobblersettingsdialog.cpp
    scrobblerhandler.cpp
    scrobblerfactory.cpp
    scrobblercache.cpp
    scrobbler.cpp
    scrobblersettingsdialog.ui
    translations/translations.qrc
)

add_library(scrobbler MODULE ${libscrobbler_SRCS})
target_link_libraries(scrobbler PRIVATE Qt6::Widgets Qt6::Network libqmmpui libqmmp)
install(TARGETS scrobbler DESTINATION ${PLUGIN_DIR}/General)
