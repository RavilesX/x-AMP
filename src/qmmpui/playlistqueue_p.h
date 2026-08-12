/***************************************************************************
 *   Copyright (C) 2026 by x-AMP contributors                              *
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

#ifndef PLAYLISTQUEUE_P_H
#define PLAYLISTQUEUE_P_H

#include <QHash>
#include <QList>
#include <QObject>

class PlayListContainer;
class PlayListModel;
class PlayListTrack;

/*! @internal
 * @brief The play queue, shared by every playlist.
 *
 * Upstream keeps a queue per playlist, so a track queued in one of them is
 * only ever played while that playlist is the one running. There is one
 * queue here instead: it holds tracks from any playlist, in a single order,
 * and the number shown beside a queued track is its place in that order.
 *
 * Which playlist a queued track belongs to still matters -- playing it means
 * making that playlist the current one -- so every entry is kept with the
 * container holding it, and each container is registered by the model that
 * owns it (see setOwner()).
 *
 * Not part of the installed API: playlists reach it through PlayListModel
 * and interfaces through PlayListManager.
 */
class PlayListQueue : public QObject
{
    Q_OBJECT
public:
    static PlayListQueue *instance();

    /*! Names the model \b container belongs to. A null model forgets it. */
    void setOwner(PlayListContainer *container, PlayListModel *model);
    /*! The playlist \b track is queued from, or null if it is not queued. */
    PlayListModel *owner(const PlayListTrack *track) const;

    /*! Appends \b track, held by \b container, to the end of the queue. */
    void enqueue(PlayListContainer *container, PlayListTrack *track);
    void remove(PlayListTrack *track);
    /*! Drops everything \b container had queued. */
    void removeAll(PlayListContainer *container);
    /*! Drops everything \b model had queued, before it is destroyed. */
    void removeAll(PlayListModel *model);
    /*!
     * Makes \b container's part of the queue exactly \b tracks, in that
     * order, without moving it past the entries of other playlists. Used
     * after a sort or a refresh has rebuilt a playlist.
     */
    void restore(PlayListContainer *container, const QList<PlayListTrack *> &tracks);
    /*! Puts the whole queue into the order of \b tracks. Unknown tracks are ignored. */
    void reorder(const QList<PlayListTrack *> &tracks);
    void clear();

    /*! What \b container has queued, in queue order. */
    QList<PlayListTrack *> tracks(const PlayListContainer *container) const;
    /*! The whole queue, in order. */
    QList<PlayListTrack *> tracks() const;
    PlayListTrack *head() const;
    /*! The playlist the head belongs to, or null on an empty queue. */
    PlayListModel *headOwner() const;
    /*! Removes the head and returns it. */
    PlayListTrack *takeHead();
    bool isEmpty() const { return m_queue.isEmpty(); }
    int count() const { return m_queue.count(); }
    bool contains(const PlayListTrack *track) const;

signals:
    /*! The queue changed, so every playlist has to redraw its numbers. */
    void changed();
    /*! The queue reached a track of \b model, which has to be made current. */
    void activationRequested(PlayListModel *model);

private:
    /*! Writes each queued track's place back onto the track itself. */
    void updateIndexes();

    struct Entry
    {
        PlayListContainer *container = nullptr;
        PlayListTrack *track = nullptr;
    };

    QList<Entry> m_queue;
    QHash<PlayListContainer *, PlayListModel *> m_owners;
};

#endif
