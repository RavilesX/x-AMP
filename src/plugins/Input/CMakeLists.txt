option(USE_MAD     "mad decoder (mpeg plugin)" ON)
option(USE_MPG123  "mpg123 decoder (mpeg plugin)" ON)
option(USE_FLAC    "flac plugin" ON)
option(USE_VORBIS  "ogg vorbis plugin" ON)
option(USE_FFMPEG  "ffmpeg plugin" ON)
option(USE_MPC     "mpc plugin" ON)
option(USE_SNDFILE "sndfile plugin" ON)
option(USE_WAVPACK "wavpack plugin" ON)
option(USE_XMP     "xmp plugin" ON)
option(USE_AAC     "aac plugin" ON)
option(USE_CUE     "cue plugin" ON)
option(USE_CDA     "cd audio support" ON)
option(USE_MIDI    "midi support" ON)
option(USE_GME     "game music plugin" ON)
option(USE_OPUS    "opus plugin" ON)
option(USE_SID     "sid plugin" ON)
option(USE_ARCHIVE "archive plugin" ON)


if((USE_MAD OR USE_MPG123) AND TAGLIB_FOUND)
    add_subdirectory(mpeg)
endif((USE_MAD OR USE_MPG123) AND TAGLIB_FOUND)

if(USE_FLAC AND TAGLIB_FOUND)
    add_subdirectory(flac)
endif(USE_FLAC AND TAGLIB_FOUND)

if(USE_VORBIS AND TAGLIB_FOUND)
    add_subdirectory(vorbis)
endif(USE_VORBIS AND TAGLIB_FOUND)

if(USE_FFMPEG AND TAGLIB_FOUND)
    add_subdirectory(ffmpeg)
endif(USE_FFMPEG AND TAGLIB_FOUND)

if(USE_MPC AND TAGLIB_FOUND)
    add_subdirectory(mpc)
endif(USE_MPC AND TAGLIB_FOUND)

if(USE_SNDFILE AND TAGLIB_FOUND)
   add_subdirectory(sndfile)
endif(USE_SNDFILE AND TAGLIB_FOUND)

if(USE_WAVPACK)
    add_subdirectory(wavpack)
endif(USE_WAVPACK)

if(USE_XMP)
   add_subdirectory(xmp)
endif(USE_XMP)

if(USE_AAC AND TAGLIB_FOUND)
   add_subdirectory(aac)
endif(USE_AAC AND TAGLIB_FOUND)

if(USE_CUE)
add_subdirectory(cue)
endif(USE_CUE)

if(USE_CDA)
add_subdirectory(cdaudio)
endif(USE_CDA)

if(USE_MIDI)
add_subdirectory(wildmidi)
endif(USE_MIDI)

if(USE_GME)
    add_subdirectory(gme)
endif(USE_GME)

if(USE_OPUS AND TAGLIB_FOUND)
    add_subdirectory(opus)
endif(USE_OPUS AND TAGLIB_FOUND)

if(USE_SID)
    add_subdirectory(sid)
endif(USE_SID)

if(USE_ARCHIVE AND TAGLIB_FOUND)
    add_subdirectory(archive)
endif(USE_ARCHIVE AND TAGLIB_FOUND)
