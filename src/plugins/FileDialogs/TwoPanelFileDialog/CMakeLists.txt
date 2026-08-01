project(libtwopanelfiledialog)

set(libtwopanelfiledialog_SRCS
    twopanelfiledialogimpl.cpp
    twopanelfiledialog.cpp
    twopanelfiledialog.ui
    translations/translations.qrc
)

add_library(twopanelfiledialog MODULE ${libtwopanelfiledialog_SRCS})
target_link_libraries(twopanelfiledialog PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS twopanelfiledialog DESTINATION ${PLUGIN_DIR}/FileDialogs)
