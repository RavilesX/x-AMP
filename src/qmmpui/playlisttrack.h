/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#ifndef PLAYLISTTRACK_H
#define PLAYLISTTRACK_H

#include <QStringList>
#include <qmmp/trackinfo.h>
#include "playlistitem.h"
#include "qmmpui_export.h"

class QmmpUiSettings;
class PlayListHeaderModel;
class MetaDataHelper;
class PlayListTrackPrivate;

/** @brief The PlayListTrack class provides a track for use with the PlayListModel class.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMPUI_EXPORT PlayListTrack : public TrackInfo, public PlayListItem
{
    Q_DECLARE_PRIVATE(PlayListTrack)
public:
    /*!
     * Constructs an empty plalist item.
     */
    PlayListTrack();
    /*!
     * Constructs a new PlayListTrack that is a copy of the given track \b other
     */
    explicit PlayListTrack(const PlayListTrack &other);
    /*!
     * Constructs a new PlayListTrack that is a copy of the given track pointer \b other
     */
    explicit PlayListTrack(const PlayListTrack *other);
    /*!
     * Move-constructs a PlayListTrack instance, making it point at the same object that \b other was pointing to.
     */
    explicit PlayListTrack(PlayListTrack &&other) noexcept;
    /*!
     * Constructs plalist item with given metadata.
     * \param info Media file information.
     */
    PlayListTrack(const TrackInfo &info);
    /*!
     * Move-constructs a PlayListTrack instance, making it point at the same object that \b other was pointing to.
     */
    PlayListTrack(TrackInfo &&info);
    /*!
     * Object destructor.
     */
    virtual ~PlayListTrack();
    /*!
     * Assigns \b other to this track and returns a reference to this track.
     */
    PlayListTrack &operator=(const PlayListTrack &other);
    /*!
     * Move-assigns \b p to this AudioParameters instance.
     */
    PlayListTrack &operator=(PlayListTrack &&other) noexcept;
    /*!
     * Returns formatted title of the item.
     * \param column Number of column.
     */
    QString formattedTitle(int column) const override;
    /*!
     * Returns the list of the formatted titles for all columns.
     */
    QStringList formattedTitles() const override;
    /*!
     *  Returns formatted length of the item.
     */
    QString formattedDuration() const override;
    /*!
     *  Updates current metadata.
     *  \param info Track information.
     */
    void updateMetaData(const TrackInfo &info);
    /*!
     * Gets new metadata from file (works for local files only).
     */
    void updateMetaData();
    /*!
     * Returns internal name of the parent group.
     */
    QString groupName() const override;
    /*!
     * Returns \b false.
     */
    bool isGroup() const override;
    /*!
     * Returns the index of the track.
     */
    int trackIndex() const override;
    /*!
     * Returns index of the queued track. Unqueued track returns \b -1.
     */
    int queuedIndex() const;
    /*!
     * Returns \b true if track is queued. Otherwise returns \b false.
     */
    bool isQueued() const;
    /*!
     * Prepares for usage.
     * Increases reference counter.
     */
    void beginUsage();
    /*!
     * End of usage.
     * Decreases reference counter.
     */
    void endUsage();
    /*!
     * Shedules track object for deletion.
     */
    void deleteLater();
    /*!
     * Returns \b true if the track object is sheduled for deletion.
     */
    bool isSheduledForDeletion() const;
    /*!
     * Indicates that instance is currently busy in some kind of operation (tags editing etc.)
     * and can't be deleted at the moment. Call \b deleteLater() instead of delete operator call.
     */
    bool isUsed() const;

private:
    PlayListTrackPrivate *d_ptr;
    friend class PlayListContainer;
    friend class NormalContainer;
    friend class GroupedContainer;
    friend class PlayListQueue;
};

#endif
