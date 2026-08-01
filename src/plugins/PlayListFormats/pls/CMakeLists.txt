project(libplsplaylistformat)

set(libplsplaylistformat_SRCS
    plsplaylistformat.cpp
)

add_library(plsplaylistformat MODULE ${libplsplaylistformat_SRCS})
target_link_libraries(plsplaylistformat PRIVATE Qt6::Widgets libqmmp libqmmpui)
install(TARGETS plsplaylistformat DESTINATION ${PLUGIN_DIR}/PlayListFormats)
