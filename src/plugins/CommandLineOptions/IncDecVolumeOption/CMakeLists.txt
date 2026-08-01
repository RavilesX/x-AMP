project(libincdecvolumeoption)

set(libincdecvolumeoption_SRCS
    incdecvolumeoption.cpp
    translations/translations.qrc
)

add_library(incdecvolumeoption MODULE ${libincdecvolumeoption_SRCS})
target_link_libraries(incdecvolumeoption PRIVATE Qt6::Widgets libqmmpui libqmmp)
install(TARGETS incdecvolumeoption DESTINATION ${PLUGIN_DIR}/CommandLineOptions)
