project(libqtmultimedia)

SET(libqtmultimedia_SRCS
  outputqtmultimedia.cpp
  outputqtmultimediafactory.cpp
  qtmultimediasettingsdialog.cpp
  qtmultimediasettingsdialog.ui
  translations/translations.qrc
)

add_library(qtmultimedia MODULE ${libqtmultimedia_SRCS} ${libqtmultimedia_UIS_H} ${libqtmultimedia_RCC_SRCS})
target_link_libraries(qtmultimedia PRIVATE Qt6::Widgets Qt6::Multimedia libqmmp)
install(TARGETS qtmultimedia DESTINATION ${PLUGIN_DIR}/Output)
