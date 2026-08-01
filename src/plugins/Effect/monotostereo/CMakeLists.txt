project(libmonotostereo)

set(libmonotostereo_SRCS
    monotostereoplugin.cpp
    effectmonotostereofactory.cpp
    translations/translations.qrc
)

add_library(monotostereo MODULE ${libmonotostereo_SRCS})
target_link_libraries(monotostereo PRIVATE Qt6::Widgets libqmmp)
install(TARGETS monotostereo DESTINATION ${PLUGIN_DIR}/Effect)
