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
#include "playlistqueue_p.h"
#include "playlistcontainer_p.h"

//The queue is not the container's own any more: one queue is shared by
//every playlist (see PlayListQueue). These stay as they were from the
//container's point of view -- they only ever touch its own tracks -- so
//the callers scattered through the containers need no changes.

PlayListContainer::~PlayListContainer()
{
    PlayListQueue::instance()->setOwner(this, nullptr);
}

void PlayListContainer::enqueue(PlayListTrack *track)
{
    PlayListQueue::instance()->enqueue(this, track);
}

void PlayListContainer::removeFromQueue(PlayListTrack *track)
{
    PlayListQueue::instance()->remove(track);
}

void PlayListContainer::clearQueue()
{
    PlayListQueue::instance()->removeAll(this);
}

void PlayListContainer::restoreQueue(const QList<PlayListTrack *> &tracks)
{
    PlayListQueue::instance()->restore(this, tracks);
}

QList<PlayListTrack *> PlayListContainer::queuedTracks() const
{
    return PlayListQueue::instance()->tracks(this);
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
