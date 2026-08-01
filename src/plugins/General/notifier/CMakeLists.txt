project(libnotifier)

set(libnotifier_SRCS
    notifiersettingsdialog.cpp
    notifier.cpp
    notifierfactory.cpp
    popupwidget.cpp
    notifiersettingsdialog.ui
    translations/translations.qrc
    notifier_images.qrc
)

add_library(notifier MODULE ${libnotifier_SRCS} ${libnotifier_UIS_H} ${libnotifier_RCC_SRCS})
if(X11_FOUND)
    add_definitions(-DX11_FOUND)
    target_link_libraries(notifier PRIVATE Qt6::Widgets PkgConfig::X11 libqmmpui libqmmp)
else(X11_FOUND)
    target_link_libraries(notifier PRIVATE Qt6::Widgets libqmmpui libqmmp)
endif(X11_FOUND)
install(TARGETS notifier DESTINATION ${PLUGIN_DIR}/General)
