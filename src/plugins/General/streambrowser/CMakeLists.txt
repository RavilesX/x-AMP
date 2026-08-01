project(libstreambrowser)

set(libstreambrowser_SRCS
    streambrowser.cpp
    streambrowserfactory.cpp
    streamwindow.cpp
    editstreamdialog.cpp
    streamwindow.ui
    editstreamdialog.ui
    translations/translations.qrc
)

add_library(streambrowser MODULE ${libstreambrowser_SRCS})
target_link_libraries(streambrowser PRIVATE Qt6::Widgets Qt6::Network libqmmpui libqmmp)
install(TARGETS streambrowser DESTINATION ${PLUGIN_DIR}/General)
