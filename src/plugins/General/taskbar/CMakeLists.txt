project(libtaskbar)

set(libtaskbar_SRCS
    qwinevent.cpp
    qwineventfilter.cpp
    qwintaskbarbutton.cpp
    qwintaskbarprogress.cpp
    taskbarfactory.cpp
    taskbarhelper.cpp
    translations/translations.qrc
)

add_library(taskbar MODULE ${libtaskbar_SRCS})
target_link_libraries(taskbar PRIVATE Qt6::Widgets libqmmpui libqmmp -lole32 -lshlwapi -lshell32 -ldwmapi -lgdi32 -luuid)
install(TARGETS taskbar DESTINATION ${PLUGIN_DIR}/General)
