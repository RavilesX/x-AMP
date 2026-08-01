project(libconverter)

set(libconverter_SRCS
    converterfactory.cpp
    converter.cpp
    converterhelper.cpp
    converterdialog.cpp
    converterpreseteditor.cpp
    converterdialog.ui
    converterpreseteditor.ui
    translations/translations.qrc
    presets.qrc
)

add_library(converter MODULE ${libconverter_SRCS})
target_link_libraries(converter PRIVATE Qt6::Widgets libqmmpui libqmmp PkgConfig::TAGLIB)
install(TARGETS converter DESTINATION ${PLUGIN_DIR}/General)
