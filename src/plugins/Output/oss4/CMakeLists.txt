project(liboss4)

INCLUDE(CheckCXXSourceCompiles)

#oss4
include_directories(/usr/lib/oss/include /usr/local/lib/oss/include)
check_include_file(sys/soundcard.h SYS_SOUNDCARD_H_FOUND)

if(SYS_SOUNDCARD_H_FOUND)
    add_definitions(-DHAVE_SYS_SOUNDCARD_H)
    set(CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_SOUNDCARD_H)
endif(SYS_SOUNDCARD_H_FOUND)


set(CMAKE_REQUIRED_INCLUDES 
   /usr/lib/oss/include/
   /usr/local/lib/oss/include 
   ${CMAKE_REQUIRED_INCLUDES})

check_cxx_source_compiles("
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#else
#include <soundcard.h>
#endif

#ifndef SNDCTL_DSP_COOKEDMODE
#error \\\"oss test failed\\\"
#endif


int main (int argc, char **argv)
{
   exit(0);
}" OSS4_SUPPORT)

if(OSS4_SUPPORT)
    set(OSS4_FOUND TRUE CACHE INTERNAL "oss4")
endif(OSS4_SUPPORT)

set(liboss4_SRCS
  outputoss4factory.cpp
  outputoss4.cpp
  oss4settingsdialog.cpp
  oss4settingsdialog.ui
  translations/translations.qrc
)

if(OSS4_FOUND)
    add_library(oss4 MODULE ${liboss4_SRCS})
    target_link_libraries(oss4 PRIVATE Qt6::Widgets libqmmp)
    install(TARGETS oss4 DESTINATION ${PLUGIN_DIR}/Output)
endif(OSS4_FOUND)
