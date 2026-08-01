project(libudisks)

set(libudisks_SRCS
    udisksfactory.cpp
    udisksmanager.cpp
    udisksplugin.cpp
    udisksdevice.cpp
    udiskssettingsdialog.cpp
    udiskssettingsdialog.ui
    translations/translations.qrc
)

add_library(udisks MODULE ${libudisks_SRCS})
target_link_libraries(udisks PRIVATE Qt6::Widgets Qt6::DBus libqmmpui libqmmp)
install(TARGETS udisks DESTINATION ${PLUGIN_DIR}/General)
