project(xui)

set(libxui_SRCS
    xuicontrols.cpp
    xuidialogs.cpp
    xuidock.cpp
    xuiequalizercard.cpp
    xuifactory.cpp
    xuiicons.cpp
    xuilistview.cpp
    xuimainwindow.cpp
    xuiplayercard.cpp
    xuiplaylistcard.cpp
    xuisettings.cpp
    xuistyle.cpp
    xuivisualization.cpp
    xuiwindow.cpp
    translations/translations.qrc
    images/images.qrc
)

add_library(xui MODULE ${libxui_SRCS})
target_link_libraries(xui PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS xui DESTINATION ${PLUGIN_DIR}/Ui)
