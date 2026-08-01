project(libcue)

if(USE_ENCA AND ENCA_FOUND)
    add_definitions(-DWITH_ENCA)
endif(USE_ENCA AND ENCA_FOUND)

SET(libcue_SRCS
  decoder_cue.cpp
  decodercuefactory.cpp
  cuesettingsdialog.cpp
  cuefile.cpp
  cuemetadatamodel.cpp
  translations/translations.qrc
  cuesettingsdialog.ui
)

add_library(cue MODULE ${libcue_SRCS} ${libcue_RCC_SRCS})
if(USE_ENCA AND ENCA_FOUND)
    target_link_libraries(cue PRIVATE Qt6::Widgets libqmmp PkgConfig::ENCA)
else(USE_ENCA AND ENCA_FOUND)
    target_link_libraries(cue PRIVATE Qt6::Widgets libqmmp)
endif(USE_ENCA AND ENCA_FOUND)
install(TARGETS cue DESTINATION ${PLUGIN_DIR}/Input)
