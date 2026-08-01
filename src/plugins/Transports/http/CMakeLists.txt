project(libhttp)

# libcurl
pkg_search_module(CURL libcurl>=7.32.0 IMPORTED_TARGET)

# libenca
if(USE_ENCA AND ENCA_FOUND)
    add_definitions(-DWITH_ENCA)
endif(USE_ENCA AND ENCA_FOUND)

set(libhttp_SRCS
    httpstreamreader.cpp
    httpinputfactory.cpp
    httpinputsource.cpp
    httpsettingsdialog.cpp
    httpsettingsdialog.ui
    translations/translations.qrc
)

if(CURL_FOUND)
    add_library(http MODULE ${libhttp_SRCS})
    target_link_libraries(http PRIVATE Qt6::Widgets libqmmp PkgConfig::CURL)

    if(USE_ENCA AND ENCA_FOUND)
        target_link_libraries(http PRIVATE PkgConfig::ENCA)      
    endif(USE_ENCA AND ENCA_FOUND)

    install(TARGETS http DESTINATION ${PLUGIN_DIR}/Transports)
endif(CURL_FOUND)
