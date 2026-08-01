project(libgme)

# libgme
pkg_check_modules(GME libgme>=0.6.3 IMPORTED_TARGET)

SET(libgme_SRCS
  decoder_gme.cpp
  decodergmefactory.cpp
  gmehelper.cpp
  gmesettingsdialog.cpp
  gmesettingsdialog.ui
  translations/translations.qrc
)

if(GME_FOUND)
    add_library(gme MODULE ${libgme_SRCS} ${libgme_RCC_SRCS})
    target_link_libraries(gme PRIVATE Qt6::Widgets libqmmp PkgConfig::GME)
    install(TARGETS gme DESTINATION ${PLUGIN_DIR}/Input)
endif(GME_FOUND)
