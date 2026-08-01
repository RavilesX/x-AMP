project(libqmmpfiledialog)

set(libqmmpfiledialog_SRCS
    qmmpfiledialogimpl.cpp
    qmmpfiledialog.cpp
    qmmpfiledialog.ui
    translations/translations.qrc
)

add_library(qmmpfiledialog MODULE ${libqmmpfiledialog_SRCS})
target_link_libraries(qmmpfiledialog PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS qmmpfiledialog DESTINATION ${PLUGIN_DIR}/FileDialogs)
