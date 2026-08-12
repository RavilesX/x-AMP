#ifndef PLAYLISTTRACK_P_H
#define PLAYLISTTRACK_P_H

#include "playlisttrack.h"

class QmmpUiSettings;
class MetaDataHelper;

class PlayListTrackPrivate
{
    Q_DECLARE_PUBLIC(PlayListTrack)
public:
    PlayListTrackPrivate(PlayListTrack *track);
    void formatTitle(int column) const;
    void formatGroup() const;

private:
    PlayListTrack *q_ptr;
    mutable QStringList formattedTitles;
    mutable QString formattedLength;
    mutable QString group;
    mutable QStringList titleFormats;
    mutable QString groupFormat;
    QmmpUiSettings *settings;
    int refCount = 0;
    bool sheduledForDeletion = false;
    MetaDataHelper *helper;
    int queuedIndex = -1;
    int trackIndex = -1;
    friend class PlayListContainer;
    friend class NormalContainer;
    friend class GroupedContainer;
    friend class PlayListQueue;
};

#endif // PLAYLISTTRACK_P_H
