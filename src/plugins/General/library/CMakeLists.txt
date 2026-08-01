project(liblibrary)

set(liblibrary_SRCS
    library.cpp
    libraryfactory.cpp
    librarymodel.cpp
    librarywidget.cpp
    librarysettingsdialog.cpp
    librarywidget.ui
    librarysettingsdialog.ui
    translations/translations.qrc
)

add_library(library MODULE ${liblibrary_SRCS})
target_link_libraries(library PRIVATE Qt6::Widgets Qt6::Sql libqmmpui libqmmp)
install(TARGETS library DESTINATION ${PLUGIN_DIR}/General)
