project(liblistenbrainz)

SET(liblistenbrainz_SRCS
    listenbrainzsettingsdialog.cpp
    listenbrainzfactory.cpp
    payloadcache.cpp
    listenbrainz.cpp
    listenbrainzsettingsdialog.ui
    translations/translations.qrc
)

add_library(listenbrainz MODULE ${liblistenbrainz_SRCS})
target_link_libraries(listenbrainz PRIVATE Qt6::Widgets Qt6::Network libqmmpui libqmmp)
install(TARGETS listenbrainz DESTINATION ${PLUGIN_DIR}/General)
