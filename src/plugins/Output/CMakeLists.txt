option(USE_ALSA         "alsa plugin" ON)
option(USE_JACK         "jack plugin" ON)
option(USE_OSS          "oss plugin" OFF)
option(USE_OSS4         "oss4 plugin" ON)
option(USE_PULSE        "pulseaudio plugin" ON)
option(USE_PIPEWIRE     "pipewire plugin" ON)
option(USE_NULL         "null output plugin" ON)
option(USE_WAVEOUT      "Win32 waveout plugin" ON)
option(USE_DSOUND       "DirectSound plugin" ON)
option(USE_QTMULTIMEDIA "Qt Multimedia output plugin" ON)
option(USE_WASAPI       "WASAPI plugin" ON)
option(USE_SHOUT        "Icecast plugin" ON)

if(USE_ALSA)
    add_subdirectory(alsa)
endif(USE_ALSA)

if(USE_JACK)
    add_subdirectory(jack)
endif(USE_JACK)

if(USE_OSS)
    add_subdirectory(oss)
endif(USE_OSS)

if(USE_OSS4)
    add_subdirectory(oss4)
endif(USE_OSS4)

if(USE_PULSE)
    add_subdirectory(pulseaudio)
endif(USE_PULSE)

if(USE_PIPEWIRE)
    add_subdirectory(pipewire)
endif(USE_PIPEWIRE)

if(USE_NULL)
    add_subdirectory(null)
endif(USE_NULL)

if(USE_WAVEOUT)
    add_subdirectory(waveout)
endif(USE_WAVEOUT)

if(USE_DSOUND)
    add_subdirectory(directsound)
endif(USE_DSOUND)

if(USE_QTMULTIMEDIA AND Qt6Multimedia_FOUND)
    add_subdirectory(qtmultimedia)
endif(USE_QTMULTIMEDIA AND Qt6Multimedia_FOUND)

if(USE_WASAPI)
    add_subdirectory(wasapi)
endif(USE_WASAPI)

if(USE_SHOUT)
    add_subdirectory(shout)
endif(USE_SHOUT)
