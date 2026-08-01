project(libcopypaste)

set(libcopypaste_SRCS
    copypaste.cpp
    copypastefactory.cpp
    translations/translations.qrc
)

add_library(copypaste MODULE ${libcopypaste_SRCS})
target_link_libraries(copypaste PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS copypaste DESTINATION ${PLUGIN_DIR}/General)
