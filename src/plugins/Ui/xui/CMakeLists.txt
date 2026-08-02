project(xui)

set(libxui_SRCS
    xuicontrols.cpp
    xuifactory.cpp
    xuiicons.cpp
    xuimainwindow.cpp
    xuiplayercard.cpp
    xuivisualization.cpp
    translations/translations.qrc
)

add_library(xui MODULE ${libxui_SRCS})
target_link_libraries(xui PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS xui DESTINATION ${PLUGIN_DIR}/Ui)
