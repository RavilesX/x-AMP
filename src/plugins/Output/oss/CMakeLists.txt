project(liboss)

#OSS
check_include_file(sys/soundcard.h SYS_SOUNDCARD_H_FOUND)
if(SYS_SOUNDCARD_H_FOUND)
    add_definitions(-DHAVE_SYS_SOUNDCARD_H)
endif(SYS_SOUNDCARD_H_FOUND)

set(liboss_SRCS
  outputossfactory.cpp
  outputoss.cpp
  osssettingsdialog.cpp
  osssettingsdialog.ui
  translations/translations.qrc
)

add_library(oss MODULE ${liboss_SRCS})
target_link_libraries(oss PRIVATE Qt6::Widgets libqmmp)
install(TARGETS oss DESTINATION ${PLUGIN_DIR}/Output)
