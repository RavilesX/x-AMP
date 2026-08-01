project(librdetect)

SET(librdetect_SRCS
    rdetectfactory.cpp
    removablehelper.cpp
    rdetectsettingsdialog.cpp
    rdetectsettingsdialog.ui
    translations/translations.qrc
)

add_library(rdetect MODULE ${librdetect_SRCS})
target_link_libraries(rdetect PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS rdetect DESTINATION ${PLUGIN_DIR}/General)
