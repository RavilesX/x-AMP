project(libstereo)

set(libstereo_SRCS
    stereoplugin.cpp
    stereosettingsdialog.cpp
    effectstereofactory.cpp
    stereosettingsdialog.ui
    translations/translations.qrc
)

add_library(stereo MODULE ${libstereo_SRCS})
target_link_libraries(stereo Qt6::Widgets libqmmp)
install(TARGETS stereo DESTINATION ${PLUGIN_DIR}/Effect)
