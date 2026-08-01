option(USE_SOXR         "SoX resampler plugin" ON)
option(USE_BS2B         "bs2b plugin" ON)
option(USE_LADSPA       "ladspa plugin" ON)
option(USE_CROSSFADE    "crossfade plugin" ON)
option(USE_STEREO       "extra stereo plugin" ON)
option(USE_FILEWRITER   "file writer plugin" ON)
option(USE_MONOTOSTEREO "mono to stereo converter plugin" ON)

if(USE_SOXR)
    add_subdirectory(soxr)
endif(USE_SOXR)

if(USE_BS2B)
    add_subdirectory(bs2b)
endif(USE_BS2B)

if(USE_LADSPA)
   add_subdirectory(ladspa)
endif(USE_LADSPA)

if(USE_CROSSFADE)
    add_subdirectory(crossfade)
endif(USE_CROSSFADE)

if(USE_STEREO)
    add_subdirectory(stereo)
endif(USE_STEREO)

if(USE_FILEWRITER)
    add_subdirectory(filewriter)
endif(USE_FILEWRITER)

if(USE_MONOTOSTEREO)
    add_subdirectory(monotostereo)
endif(USE_MONOTOSTEREO)
