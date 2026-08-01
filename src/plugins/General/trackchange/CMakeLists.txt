project(libtrackchange)

set(libtrackchange_SRCS
    trackchange.cpp
    trackchangefactory.cpp
    trackchangesettingsdialog.cpp
    trackchangesettingsdialog.ui
    translations/translations.qrc
)

add_library(trackchange MODULE ${libtrackchange_SRCS})
target_link_libraries(trackchange PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS trackchange DESTINATION ${PLUGIN_DIR}/General)
