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

#include "playlisttrack_p.h"
#include "playlistqueue_p.h"

PlayListQueue *PlayListQueue::instance()
{
    static PlayListQueue queue;
    return &queue;
}

void PlayListQueue::setOwner(PlayListContainer *container, PlayListModel *model)
{
    if(model)
    {
        m_owners.insert(container, model);
        return;
    }
    m_owners.remove(container);
    removeAll(container);
}

PlayListModel *PlayListQueue::owner(const PlayListTrack *track) const
{
    for(const Entry &entry : m_queue)
    {
        if(entry.track == track)
            return m_owners.value(entry.container);
    }
    return nullptr;
}

void PlayListQueue::enqueue(PlayListContainer *container, PlayListTrack *track)
{
    if(contains(track))
        return;
    m_queue.append({ container, track });
    updateIndexes();
    emit changed();
}

void PlayListQueue::remove(PlayListTrack *track)
{
    const qsizetype removed = m_queue.removeIf([track](const Entry &entry) {
        return entry.track == track;
    });
    if(!removed)
        return;
    track->d_ptr->queuedIndex = -1;
    updateIndexes();
    emit changed();
}

void PlayListQueue::removeAll(PlayListContainer *container)
{
    const qsizetype removed = m_queue.removeIf([container](const Entry &entry) {
        if(entry.container != container)
            return false;
        entry.track->d_ptr->queuedIndex = -1;
        return true;
    });
    if(!removed)
        return;
    updateIndexes();
    emit changed();
}

void PlayListQueue::removeAll(PlayListModel *model)
{
    const QList<PlayListContainer *> containers = m_owners.keys(model);
    for(PlayListContainer *container : containers)
        removeAll(container);
}

void PlayListQueue::restore(PlayListContainer *container, const QList<PlayListTrack *> &tracks)
{
    //the entries this container already holds keep their places in the
    //queue; only what they point at is written again, in the given order
    QList<PlayListTrack *> wanted = tracks;
    for(auto it = m_queue.begin(); it != m_queue.end();)
    {
        if(it->container != container)
        {
            ++it;
            continue;
        }
        if(wanted.isEmpty())
        {
            it->track->d_ptr->queuedIndex = -1;
            it = m_queue.erase(it);
            continue;
        }
        it->track = wanted.takeFirst();
        ++it;
    }
    //anything left over was not queued before and goes to the end
    for(PlayListTrack *track : std::as_const(wanted))
        m_queue.append({ container, track });

    updateIndexes();
    emit changed();
}

void PlayListQueue::reorder(const QList<PlayListTrack *> &tracks)
{
    QList<Entry> reordered;
    reordered.reserve(m_queue.count());
    for(PlayListTrack *track : tracks)
    {
        for(const Entry &entry : std::as_const(m_queue))
        {
            if(entry.track == track)
            {
                reordered.append(entry);
                break;
            }
        }
    }
    if(reordered.count() != m_queue.count())
        return; //an incomplete order would drop tracks: leave the queue alone

    m_queue = reordered;
    updateIndexes();
    emit changed();
}

void PlayListQueue::clear()
{
    if(m_queue.isEmpty())
        return;
    for(const Entry &entry : std::as_const(m_queue))
        entry.track->d_ptr->queuedIndex = -1;
    m_queue.clear();
    emit changed();
}

QList<PlayListTrack *> PlayListQueue::tracks(const PlayListContainer *container) const
{
    QList<PlayListTrack *> tracks;
    for(const Entry &entry : m_queue)
    {
        if(entry.container == container)
            tracks << entry.track;
    }
    return tracks;
}

QList<PlayListTrack *> PlayListQueue::tracks() const
{
    QList<PlayListTrack *> tracks;
    tracks.reserve(m_queue.count());
    for(const Entry &entry : m_queue)
        tracks << entry.track;
    return tracks;
}

PlayListTrack *PlayListQueue::head() const
{
    return m_queue.isEmpty() ? nullptr : m_queue.constFirst().track;
}

PlayListModel *PlayListQueue::headOwner() const
{
    return m_queue.isEmpty() ? nullptr : m_owners.value(m_queue.constFirst().container);
}

PlayListTrack *PlayListQueue::takeHead()
{
    if(m_queue.isEmpty())
        return nullptr;
    PlayListTrack *track = m_queue.takeFirst().track;
    track->d_ptr->queuedIndex = -1;
    updateIndexes();
    emit changed();
    return track;
}

bool PlayListQueue::contains(const PlayListTrack *track) const
{
    for(const Entry &entry : m_queue)
    {
        if(entry.track == track)
            return true;
    }
    return false;
}

void PlayListQueue::updateIndexes()
{
    for(int i = 0; i < m_queue.count(); ++i)
        m_queue.at(i).track->d_ptr->queuedIndex = i;
}
