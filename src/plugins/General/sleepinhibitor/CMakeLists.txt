project(libsleepinhibitor)

set(libsleepinhibitor_SRCS
    sleepinhibitor.cpp
    sleepinhibitorfactory.cpp
    translations/translations.qrc
)

add_library(sleepinhibitor MODULE ${libsleepinhibitor_SRCS})
target_link_libraries(sleepinhibitor PRIVATE Qt6::Widgets Qt6::DBus libqmmpui libqmmp)
install(TARGETS sleepinhibitor DESTINATION ${PLUGIN_DIR}/General)
