project(libgnomehotkey)

set(libgnomehotkey_SRCS
    gnomehotkeyfactory.cpp
    mediakeys.cpp
    translations/translations.qrc
)

add_library(gnomehotkey MODULE ${libgnomehotkey_SRCS} ${libgnomehotkey_RCC_SRCS})
target_link_libraries(gnomehotkey PRIVATE Qt6::Widgets Qt6::DBus libqmmpui libqmmp)
install(TARGETS gnomehotkey DESTINATION ${PLUGIN_DIR}/General)
