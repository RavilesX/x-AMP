project(libhistory)

set(libhistory_SRCS
    history.cpp
    historyfactory.cpp
    historysettingsdialog.cpp
    progressbaritemdelegate.cpp
    historywindow.cpp
    historysettingsdialog.ui
    historywindow.ui
    translations/translations.qrc
)

add_library(history MODULE ${libhistory_SRCS})
target_link_libraries(history PRIVATE Qt6::Widgets Qt6::Sql libqmmpui libqmmp)
install(TARGETS history DESTINATION ${PLUGIN_DIR}/General)
