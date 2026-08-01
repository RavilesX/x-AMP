option(USE_MPRIS          "mpris plugin" ON)
option(USE_SCROBBLER      "scrobbler plugin" ON)
option(USE_LISTENBRAINZ   "listenbrainz plugin" ON)
option(USE_STATICON       "status icon plugin" ON)
option(USE_NOTIFIER       "notifier plugin" ON)
option(USE_LYRICS         "lyrics version" ON)
option(USE_UDISKS         "udisks plugin" ON)
option(USE_HOTKEY         "global hotkey plugin" ON)
option(USE_GNOMEHOTKEY    "gnome hotkey plugin" ON)
option(USE_FILEOPS        "fileops plugin" ON)
option(USE_COVER          "cover manager plugin" ON)
option(USE_KDENOTIFY      "kde notification plugin" ON)
option(USE_CONVERTER      "audio converter plugin" ON)
option(USE_RGSCAN         "replaygain scanner plugin" ON)
option(USE_SB             "stream browser plugin" ON)
option(USE_TRACKCHANGE    "track change plugin" ON)
option(USE_COPYPASTE      "copy/paste change plugin" ON)
option(USE_HISTORY        "history plugin" ON)
option(USE_SLEEPINHIBITOR "sleep mode inhibition plugin" ON)
option(USE_LIBRARY        "media library plugin" ON)
option(USE_TAGEDITOR      "batch tag editor plugin" ON)
option(USE_TASKBAR        "taskbar plugin (Windows)" ON)
option(USE_RDETECT        "rdetect plugin (Windows)" ON)

if(USE_MPRIS AND Qt6DBus_FOUND)
    add_subdirectory(mpris)
endif(USE_MPRIS AND Qt6DBus_FOUND)

if(USE_SCROBBLER)
    add_subdirectory(scrobbler)
endif(USE_SCROBBLER)

if(USE_LISTENBRAINZ)
    add_subdirectory(listenbrainz)
endif(USE_LISTENBRAINZ)

if(USE_STATICON)
    add_subdirectory(statusicon)
endif(USE_STATICON)

if(USE_NOTIFIER)
    add_subdirectory(notifier)
endif(USE_NOTIFIER)

if(USE_LYRICS)
    add_subdirectory(lyrics)
endif(USE_LYRICS)

if(USE_UDISKS AND Qt6DBus_FOUND)
    add_subdirectory(udisks)
endif(USE_UDISKS AND Qt6DBus_FOUND)

if(USE_HOTKEY AND (X11_FOUND OR WIN32))
    add_subdirectory(hotkey)
endif(USE_HOTKEY AND (X11_FOUND OR WIN32))

if(USE_FILEOPS)
    add_subdirectory(fileops)
endif(USE_FILEOPS)

if(USE_COVER)
    add_subdirectory(covermanager)
endif(USE_COVER)

if(USE_KDENOTIFY AND Qt6DBus_FOUND)
    add_subdirectory(kdenotify)
endif(USE_KDENOTIFY AND Qt6DBus_FOUND)

if(USE_CONVERTER AND TAGLIB_FOUND)
    add_subdirectory(converter)
endif(USE_CONVERTER AND TAGLIB_FOUND)

if(USE_RGSCAN AND TAGLIB_FOUND)
    add_subdirectory(rgscan)
endif(USE_RGSCAN AND TAGLIB_FOUND)

if(USE_SB)
    add_subdirectory(streambrowser)
endif(USE_SB)

if(USE_TRACKCHANGE)
    add_subdirectory(trackchange)
endif(USE_TRACKCHANGE)

if(USE_COPYPASTE)
    add_subdirectory(copypaste)
endif(USE_COPYPASTE)

if(USE_GNOMEHOTKEY AND Qt6DBus_FOUND)
    add_subdirectory(gnomehotkey)
endif(USE_GNOMEHOTKEY AND Qt6DBus_FOUND)

if(USE_HISTORY AND Qt6Sql_FOUND)
   add_subdirectory(history)
endif(USE_HISTORY AND Qt6Sql_FOUND)

if(USE_SLEEPINHIBITOR AND Qt6DBus_FOUND)
   add_subdirectory(sleepinhibitor)
endif(USE_SLEEPINHIBITOR AND Qt6DBus_FOUND)

if(USE_TAGEDITOR)
   add_subdirectory(batchtageditor)
endif(USE_TAGEDITOR)

if(USE_TASKBAR AND WIN32)
   add_subdirectory(taskbar)
endif(USE_TASKBAR AND WIN32)

if(USE_RDETECT AND WIN32)
   add_subdirectory(rdetect)
endif(USE_RDETECT AND WIN32)

if(USE_LIBRARY AND Qt6Sql_FOUND)
    add_subdirectory(library)
endif(USE_LIBRARY AND Qt6Sql_FOUND)
