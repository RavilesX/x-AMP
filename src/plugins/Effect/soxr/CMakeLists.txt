project(libsoxr)

pkg_check_modules(SOXR soxr IMPORTED_TARGET)

set(libsoxr_SRCS
    soxresampler.cpp
    soxrsettingsdialog.cpp
    effectsoxrfactory.cpp
    soxrsettingsdialog.ui
    translations/translations.qrc
)

if(SOXR_FOUND)
    add_library(soxr MODULE ${libsoxr_SRCS})
    target_link_libraries(soxr PRIVATE Qt6::Widgets libqmmp PkgConfig::SOXR)
    install(TARGETS soxr DESTINATION ${PLUGIN_DIR}/Effect)
endif(SOXR_FOUND)
