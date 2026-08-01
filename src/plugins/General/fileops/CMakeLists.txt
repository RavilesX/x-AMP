project(libfileops)

set(libfileops_SRCS
    fileops.cpp
    fileopsfactory.cpp
    fileopssettingsdialog.cpp
    fileopssettingsdialog.ui
    translations/translations.qrc
)

add_library(fileops MODULE ${libfileops_SRCS})
target_link_libraries(fileops PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS fileops DESTINATION ${PLUGIN_DIR}/General)
