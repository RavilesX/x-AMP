project(liblyrics)

set(liblyrics_SRCS
    lyrics.cpp
    lyricsfactory.cpp
    lyricsprovider.cpp
    lyricswidget.cpp
    lyricssettingsdialog.cpp
    ultimatelyricsparser.cpp
    lyricswidget.ui
    lyricssettingsdialog.ui
    translations/translations.qrc
    providers/providers.qrc
)

add_library(lyrics MODULE ${liblyrics_SRCS})
target_link_libraries(lyrics PRIVATE Qt6::Widgets Qt6::Network libqmmpui libqmmp)
install(TARGETS lyrics DESTINATION ${PLUGIN_DIR}/General)
