project(librgscan)

set(librgscan_SRCS
    rgscanfactory.cpp
    rgscanhelper.cpp
    rgscandialog.cpp
    rgscanner.cpp
    gain_analysis.c
    rgscandialog.ui
    translations/translations.qrc
)

add_library(rgscan MODULE ${librgscan_SRCS} ${librgscan_UIS_H})
target_link_libraries(rgscan PRIVATE Qt6::Widgets libqmmpui libqmmp PkgConfig::TAGLIB)
install(TARGETS rgscan DESTINATION ${PLUGIN_DIR}/General)
