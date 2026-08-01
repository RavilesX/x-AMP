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

#include <QRandomGenerator>
#include "playlisttrack_p.h"
#include "normalcontainer_p.h"

NormalContainer::NormalContainer()
{}

NormalContainer::~NormalContainer()
{}

void NormalContainer::addTrack(PlayListTrack *track)
{
    addTracks({ track });
}

void NormalContainer::addTracks(const QList<PlayListTrack *> &tracks)
{
    m_tracks.reserve(m_tracks.count() + tracks.count());

    for(PlayListTrack *track : std::as_const(tracks))
    {
        m_tracks.append(track);
        track->d_ptr->queuedIndex = -1;
        track->d_ptr->trackIndex = m_tracks.count() - 1;
    }
}

int NormalContainer::insertTrack(int index, PlayListTrack *track)
{
    track->d_ptr->queuedIndex = -1;
    if(index >= 0 && index < m_tracks.count())
    {
        m_tracks.insert(index, track);
        track->d_ptr->trackIndex = index;
        //update indexes
        for(int i = index; i < m_tracks.count(); ++i)
            static_cast<PlayListTrack *>(m_tracks[i])->d_ptr->trackIndex = i;
        return index;
    }

    m_tracks.append(track);
    track->d_ptr->trackIndex = m_tracks.count() - 1;
    return m_tracks.count() - 1;
}

void NormalContainer::replaceTracks(const QList<PlayListTrack *> &tracks)
{
    clearQueue();
    m_tracks.clear();
    addTracks(tracks);
}

QList<PlayListGroup *> NormalContainer::groups() const
{
    return QList<PlayListGroup *>();
}

QList<PlayListTrack *> NormalContainer::tracks() const
{
    return m_tracks;
}

int NormalContainer::trackCount() const
{
    return m_tracks.count();
}

int NormalContainer::groupCount() const
{
    return 0;
}

QList<PlayListTrack *> NormalContainer::mid(int pos, int count) const
{
    return m_tracks.mid(pos, count);
}

bool NormalContainer::isEmpty() const
{
    return m_tracks.isEmpty();
}

void NormalContainer::clearSelection()
{
    for(PlayListItem *item : std::as_const(m_tracks))
    {
        item->setSelected(false);
    }
}

int NormalContainer::indexOf(PlayListItem *item) const
{
    return m_tracks.indexOf(item);
}

PlayListTrack *NormalContainer::track(int index) const
{
   if (0 <= index && index < m_tracks.count())
       return m_tracks.at(index);

   return nullptr;
}

PlayListGroup *NormalContainer::group(int index) const
{
    Q_UNUSED(index);
    return nullptr;
}

bool NormalContainer::contains(PlayListTrack *track) const
{
    return m_tracks.contains(track);
}

void NormalContainer::removeTrack(PlayListTrack *track)
{
    removeTracks(QList<PlayListTrack *> () << track);
}

void NormalContainer::removeTracks(QList<PlayListTrack *> tracks)
{
    for(PlayListTrack *t : std::as_const(tracks))
    {
        m_tracks.removeAll(t);
        removeFromQueue(t);
    }

    for(int i = 0; i < m_tracks.count(); ++i)
        static_cast<PlayListTrack *>(m_tracks[i])->d_ptr->trackIndex = i;
}

bool NormalContainer::move(const QList<int> &indexes, int from, int to)
{
    if (from > to)
    {
        for(const int &i : std::as_const(indexes))
        {
            if (i + to - from < 0)
                break;

            m_tracks.move(i, i + to - from);
            swapTrackNumbers(&m_tracks, i, i + to - from);
        }
    }
    else
    {
        for (int i = indexes.count() - 1; i >= 0; i--)
        {
            if (indexes[i] + to - from >= m_tracks.count())
                break;

            m_tracks.move(indexes[i], indexes[i] + to - from);
            swapTrackNumbers(&m_tracks, indexes[i], indexes[i] + to - from);
        }
    }
    return true;
}

QList<PlayListTrack *> NormalContainer::takeAllTracks()
{
    clearQueue();
    return std::move(m_tracks);
}

void NormalContainer::clear()
{
    clearQueue();
    qDeleteAll(m_tracks);
    m_tracks.clear();
}

void NormalContainer::reverseList()
{
    for(int i = 0; i < m_tracks.size()/2; i++)
    {
        m_tracks.swapItemsAt(i, m_tracks.size() - i - 1);
        swapTrackNumbers(&m_tracks, i, m_tracks.size() - i - 1);
    }
}

void NormalContainer::randomizeList()
{
    QRandomGenerator *rg = QRandomGenerator::global();

    for (int i = 0; i < m_tracks.size(); i++)
        m_tracks.swapItemsAt(rg->generate() % m_tracks.size(), rg->generate() % m_tracks.size());

    for(int i = 0; i < m_tracks.count(); ++i)
        m_tracks[i]->d_ptr->trackIndex = i;
}

int NormalContainer::lineCount() const
{
    return m_tracks.count();
}

PlayListItem *NormalContainer::itemAtLine(int lineIndex) const
{
    return track(lineIndex);
}

QList<PlayListItem *> NormalContainer::itemsAtLines(int pos, int length) const
{
    QList<PlayListTrack *> tracks = m_tracks.mid(pos, length);
    QList<PlayListItem *> items;
    for(PlayListTrack *t : std::as_const(tracks))
        items << t;
    return items;
}

int NormalContainer::subIndexOfLine(int lineIndex) const
{
    Q_UNUSED(lineIndex);
    return 0;
}

int NormalContainer::trackIndexAtLine(int lineIndex) const
{
    return lineIndex;
}

bool NormalContainer::alternateColor(int lineIndex) const
{
    return lineIndex % 2;
}
