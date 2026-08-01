project(libbatchtageditor)

set(libbatchtageditor_SRCS
    batchtageditor.cpp
    batchtageditordialog.cpp
    batchtageditorfactory.cpp
    translations/translations.qrc
)

add_library(batchtageditor MODULE ${libbatchtageditor_SRCS})
target_link_libraries(batchtageditor PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS batchtageditor DESTINATION ${PLUGIN_DIR}/General)
