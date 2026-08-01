include(../../../qmmp.pri)

SUBDIRS += statusicon \
           notifier \
           lyrics \
           scrobbler \
           fileops \
           covermanager \
           streambrowser \
           trackchange \
           copypaste \
           rgscan \
           hotkey \
           listenbrainz \
           library \
           batchtageditor

unix:SUBDIRS += mpris \
                kdenotify \
                converter \
                gnomehotkey

contains(CONFIG, UDISKS_PLUGIN){
    unix:SUBDIRS += udisks
}

contains(CONFIG, SLEEPINHIBITOR_PLUGIN){
    unix:SUBDIRS += sleepinhibitor
}

contains(CONFIG, HISTORY_PLUGIN){
    SUBDIRS += history
}

win32:SUBDIRS += rdetect taskbar

TEMPLATE = subdirs
