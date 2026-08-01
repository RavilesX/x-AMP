project(libxmp)

# xmp
pkg_search_module(XMP libxmp IMPORTED_TARGET)

SET(libxmp_SRCS
  decoder_xmp.cpp
  decoderxmpfactory.cpp
  xmpmetadatamodel.cpp
  xmpsettingsdialog.cpp
  xmpmetadatamodel.cpp
  xmpsettingsdialog.ui
  translations/translations.qrc
)

if(XMP_FOUND)
    add_library(xmp MODULE ${libxmp_SRCS} ${libxmp_UIS_H})
    target_link_libraries(xmp PRIVATE Qt6::Widgets libqmmp PkgConfig::XMP)
    install(TARGETS xmp DESTINATION ${PLUGIN_DIR}/Input)
endif(XMP_FOUND)
