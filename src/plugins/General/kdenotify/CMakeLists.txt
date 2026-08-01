project(libkdenotify)

set(libkdenotify_SRCS
    kdenotify.cpp
    kdenotifyfactory.cpp
    kdenotifysettingsdialog.cpp
    kdenotifysettingsdialog.ui
    translations/translations.qrc
)

add_library(kdenotify MODULE ${libkdenotify_SRCS})
target_link_libraries(kdenotify PRIVATE Qt6::Widgets Qt6::DBus libqmmpui libqmmp)
install(TARGETS kdenotify DESTINATION ${PLUGIN_DIR}/General)
install(FILES images/empty_cover.png DESTINATION ${CMAKE_INSTALL_DATADIR}/qmmp${APP_NAME_SUFFIX}/images)
install(FILES images/app_icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/qmmp${APP_NAME_SUFFIX}/images)
