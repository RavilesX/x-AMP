project(libwaveout)

# Win32 waveout
check_include_file(windows.h WINDOWS_H_FOUND)

SET(libwaveout_SRCS
  outputwaveout.cpp
  outputwaveoutfactory.cpp
  translations/translations.qrc
)

if(WINDOWS_H_FOUND)
    add_library(waveout MODULE ${libwaveout_SRCS} ${libwaveout_RCC_SRCS} ${libwaveout_HDRS})
    target_link_libraries(waveout PRIVATE Qt6::Widgets libqmmp -lwinmm)
    install(TARGETS waveout DESTINATION ${PLUGIN_DIR}/Output)
endif(WINDOWS_H_FOUND)

