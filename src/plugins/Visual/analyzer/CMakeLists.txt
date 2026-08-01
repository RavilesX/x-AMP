project(libanalyzer)

SET(libanalyzer_SRCS
    analyzer.cpp
    analyzercolorwidget.cpp
    analyzersettingsdialog.cpp
    visualanalyzerfactory.cpp
    analyzersettingsdialog.ui
    translations/translations.qrc
)

add_library(analyzer MODULE ${libanalyzer_SRCS})
target_link_libraries(analyzer PRIVATE Qt6::Widgets libqmmp)
install(TARGETS analyzer DESTINATION ${PLUGIN_DIR}/Visual)
