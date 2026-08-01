project(libmpris)

set(libmpris_SRCS
    mpris.cpp
    mprisfactory.cpp
    root2object.cpp
    player2object.cpp
    translations/translations.qrc
)

add_library(mpris MODULE ${libmpris_SRCS})
target_link_libraries(mpris PRIVATE Qt6::Widgets Qt6::DBus libqmmpui libqmmp)
install(TARGETS mpris DESTINATION ${PLUGIN_DIR}/General)
