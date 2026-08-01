project(libcrossfade)

SET(libcrossfade_SRCS
    crossfadeplugin.cpp
    crossfadesettingsdialog.cpp
    effectcrossfadefactory.cpp
    crossfadesettingsdialog.ui
    translations/translations.qrc
)

add_library(crossfade MODULE ${libcrossfade_SRCS})
target_link_libraries(crossfade PRIVATE Qt6::Widgets libqmmp)
install(TARGETS crossfade DESTINATION ${PLUGIN_DIR}/Effect)
