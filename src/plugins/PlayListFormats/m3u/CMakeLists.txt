project(libm3uplaylistformat)

set(libm3uplaylistformat_SRCS
    m3uplaylistformat.cpp
)

add_library(m3uplaylistformat MODULE ${libm3uplaylistformat_SRCS})
target_link_libraries(m3uplaylistformat PRIVATE Qt6::Widgets libqmmp libqmmpui)
install(TARGETS m3uplaylistformat DESTINATION ${PLUGIN_DIR}/PlayListFormats)
