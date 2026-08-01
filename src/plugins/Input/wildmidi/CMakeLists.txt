project(libwildmidi)

# wildmidi
pkg_search_module(WILDMIDI wildmidi>=0.4.1 IMPORTED_TARGET)

set(libwildmidi_SRCS
  decoder_wildmidi.cpp
  decoderwildmidifactory.cpp
  wildmidisettingsdialog.cpp
  wildmidihelper.cpp
  wildmidisettingsdialog.ui
  translations/translations.qrc
)

if(WILDMIDI_FOUND)
    add_library(wildmidi MODULE ${libwildmidi_SRCS})
    target_link_libraries(wildmidi Qt6::Widgets libqmmp PkgConfig::WILDMIDI)
    install(TARGETS wildmidi DESTINATION ${PLUGIN_DIR}/Input)
endif(WILDMIDI_FOUND)
