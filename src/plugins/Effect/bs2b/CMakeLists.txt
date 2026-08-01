project(libbs2b)

set(libbs2b_SRCS
    bs2bplugin.cpp
    bs2bsettingsdialog.cpp
    effectbs2bfactory.cpp
    bs2bsettingsdialog.ui
    translations/translations.qrc
)

pkg_search_module(BS2B libbs2b>=3.0.0 IMPORTED_TARGET)

if(WIN32)
    include_directories(${BS2B_INCLUDEDIR})
endif(WIN32)

if(BS2B_FOUND)
    add_library(bs2b MODULE ${libbs2b_SRCS})
    target_link_libraries(bs2b PRIVATE Qt6::Widgets libqmmp PkgConfig::BS2B)
    install(TARGETS bs2b DESTINATION ${PLUGIN_DIR}/Effect)
endif(BS2B_FOUND)
