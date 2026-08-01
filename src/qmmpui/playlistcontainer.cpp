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

#include "playlisttrack_p.h"
#include "playlistcontainer_p.h"

PlayListTrack *PlayListContainer::dequeue()
{
    PlayListTrack *t = m_queue.dequeue();
    t->d_ptr->queuedIndex = -1;
    updateQueueIndexes();
    return t;
}

void PlayListContainer::enqueue(PlayListTrack *track)
{
    m_queue.enqueue(track);
    updateQueueIndexes();
}

void PlayListContainer::removeFromQueue(PlayListTrack *track)
{
    if(track->isQueued())
    {
        m_queue.removeAll(track);
        track->d_ptr->queuedIndex = -1;
        updateQueueIndexes();
    }
}

void PlayListContainer::clearQueue()
{
    for(int i = 0; i < m_queue.size(); ++i)
        m_queue[i]->d_ptr->queuedIndex = -1;

    m_queue.clear();
}

void PlayListContainer::restoreQueue(const QList<PlayListTrack *> &tracks)
{
    m_queue.clear();
    for(PlayListTrack *t : std::as_const(tracks))
        m_queue.enqueue(t);
    updateQueueIndexes();
}

QList<PlayListTrack *> PlayListContainer::queuedTracks() const
{
    return m_queue;
}

int PlayListContainer::linesPerGroup() const
{
    return m_linesPerGroup;
}

void PlayListContainer::setLinesPerGroup(int lines)
{
    m_linesPerGroup = lines;
}

void PlayListContainer::swapTrackNumbers(QList<PlayListTrack *> *container, int index1, int index2)
{
    PlayListTrack *track1 = container->at(index1);
    PlayListTrack *track2 = container->at(index2);
    int number = track1->trackIndex();
    track1->d_ptr->trackIndex = track2->d_ptr->trackIndex;
    track2->d_ptr->trackIndex = number;
}

void PlayListContainer::updateQueueIndexes()
{
    for(int i = 0; i < m_queue.size(); ++i)
        m_queue[i]->d_ptr->queuedIndex = i;
}
