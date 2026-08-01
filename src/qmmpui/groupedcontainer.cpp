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
#include "playlistgroup_p.h"
#include "groupedcontainer_p.h"

GroupedContainer::GroupedContainer()
{

}

void GroupedContainer::addTrack(PlayListTrack *track)
{
    int firstIndex = 0, lastIndex = 0;
    //insert if possible
    for(int i = 0; i < m_groups.count(); ++i)
    {
        if(i == 0)
        {
           firstIndex = 0;
           lastIndex = m_groups[i]->count() - 1;
        }
        else
        {
            firstIndex = lastIndex + 1;
            lastIndex = firstIndex + m_groups[i]->count() - 1;
        }

        if(track->groupName() == m_groups[i]->groupName())
        {
            m_groups[i]->d_ptr->m_trackList.append(track);
            m_groups[i]->d_ptr->m_title0.clear();
            m_groups[i]->d_ptr->m_title1.clear();
            m_tracks.insert(lastIndex + 1, track);
            track->d_ptr->trackIndex = lastIndex + 1;
            m_update = true;
            return;
        }
    }
    //create new group
    PlayListGroup *group = new PlayListGroup(track->groupName());
    group->d_ptr->m_trackList.append(track);
    m_tracks.append(track);
    m_groups.append(group);
    track->d_ptr->trackIndex = m_tracks.count() - 1;

    if(!m_update)
    {
        for(int j = 0; j < linesPerGroup(); ++j)
        {
            PlayListLine line = { .isGroup = true, .index = int(m_groups.count() - 1), .subindex = j };
            m_lines << line;
        }

        m_lines << PlayListLine { .isGroup = false, .index = int(m_tracks.count() - 1), .subindex = 0 };
    }
}

void GroupedContainer::addTracks(const QList<PlayListTrack *> &tracks)
{
    m_tracks.reserve(m_tracks.count() + tracks.count());
    for(PlayListTrack *track : std::as_const(tracks))
        addTrack(track);
}

int GroupedContainer::insertTrack(int index, PlayListTrack *track)
{
    int firstIndex = 0, lastIndex = 0;
    //insert if possible
    for(int i = 0; i < m_groups.count(); ++i)
    {
        if(i == 0)
        {
           firstIndex = 0;
           lastIndex = m_groups[i]->count() - 1;
        }
        else
        {
            firstIndex = lastIndex + 1;
            lastIndex = firstIndex + m_groups[i]->count() - 1;
        }

        if(track->groupName() == m_groups[i]->groupName() &&
                index >= firstIndex && index <= lastIndex + 1)
        {
            m_groups[i]->d_ptr->m_trackList.insert(index - firstIndex, track);
            m_groups[i]->d_ptr->m_title0.clear();
            m_groups[i]->d_ptr->m_title1.clear();
            m_tracks.insert(index, track);
            m_update = true;
            return index;
        }
    }
    //just add otherwise
    addTrack(track);
    return m_tracks.count() - 1;
}

void GroupedContainer::replaceTracks(const QList<PlayListTrack *> &tracks)
{
    for(PlayListGroup *g : std::as_const(m_groups))
    {
        g->d_ptr->m_trackList.clear();
        g->d_ptr->m_title0.clear();
        g->d_ptr->m_title1.clear();
    }
    clear();
    addTracks(tracks);
}

QList<PlayListGroup *> GroupedContainer::groups() const
{
    return m_groups;
}

QList<PlayListTrack *> GroupedContainer::tracks() const
{
    return m_tracks;
}

int GroupedContainer::groupCount() const
{
    return m_groups.count();
}

int GroupedContainer::trackCount() const
{
    return m_tracks.count();
}

QList<PlayListTrack *> GroupedContainer::mid(int pos, int count) const
{
    updateCache();
    return m_tracks.mid(pos, count);
}

bool GroupedContainer::isEmpty() const
{
    return m_tracks.isEmpty();
}

void GroupedContainer::clearSelection()
{
    for(PlayListTrack *track : std::as_const(m_tracks))
        track->setSelected(false);

    for(PlayListGroup *group : std::as_const(m_groups))
        group->setSelected(false);
}

int GroupedContainer::indexOf(PlayListItem *item) const
{
    return item->isGroup() ? m_groups.indexOf(item) : m_tracks.indexOf(item);
}

PlayListTrack *GroupedContainer::track(int index) const
{
    if (0 <= index && index < m_tracks.count())
        return m_tracks.at(index);

    return nullptr;
}

PlayListGroup *GroupedContainer::group(int index) const
{
    if (0 <= index && index < m_groups.count())
        return m_groups.at(index);

    return nullptr;
}

bool GroupedContainer::contains(PlayListTrack *track) const
{
    return m_tracks.contains(track);
}

void GroupedContainer::removeTrack(PlayListTrack *track)
{
    QList<PlayListGroup *>::iterator it = m_groups.begin();
    while(it != m_groups.end())
    {
        if((*it)->contains(track))
        {
            (*it)->d_ptr->m_trackList.removeAll(track);
            (*it)->d_ptr->m_title0.clear();
            (*it)->d_ptr->m_title1.clear();
            m_tracks.removeAll(track);
            removeFromQueue(track);
            if((*it)->isEmpty())
            {
                PlayListGroup *group = *it;
                m_groups.removeAll(group);
                delete group;
            }
            break;
        }
        ++it;
    }

    m_update = true;
}

void GroupedContainer::removeTracks(QList<PlayListTrack *> tracks)
{
    for(PlayListTrack *t : std::as_const(tracks))
        removeTrack(t);
}

bool GroupedContainer::move(const QList<int> &indexes, int from, int to)
{
    PlayListGroup *group = nullptr;
    int firstIndex = 0, lastIndex = 0;

    for(int i = 0; i < m_groups.count(); ++i)
    {
        if(i == 0)
        {
           firstIndex = 0;
           lastIndex = m_groups[i]->count() - 1;
        }
        else
        {
            firstIndex = lastIndex + 1;
            lastIndex = firstIndex + m_groups[i]->count() - 1;
        }

        if(from >= firstIndex && from <= lastIndex && to >= firstIndex && to <= lastIndex)
        {
            group = m_groups.at(i);
            break;
        }
    }

    if(!group)
        return false;

    for(int i : std::as_const(indexes))
    {
        if(i < firstIndex || i > lastIndex)
            return false;
        if(i + to - from > lastIndex)
            return false;
        if(i + to - from < firstIndex)
            return false;
    }

    if (from > to)
    {
        for(int i : std::as_const(indexes))
        {
            if (i + to - from < 0)
                break;


            m_tracks.move(i, i + to - from);
            swapTrackNumbers(&m_tracks, i, i + to - from);
            group->d_ptr->m_trackList.move(i - firstIndex, i + to - from - firstIndex);
        }
    }
    else
    {
        for (int i = indexes.count() - 1; i >= 0; i--)
        {
            if (indexes[i] + to - from >= m_tracks.count())
                break;

            m_tracks.move(indexes[i], indexes[i] + to - from);
            swapTrackNumbers(&m_tracks,indexes[i], indexes[i] + to - from);
            group->d_ptr->m_trackList.move(indexes[i] - firstIndex, indexes[i] + to - from - firstIndex);
        }
    }
    return true;
}

QList<PlayListTrack *> GroupedContainer::takeAllTracks()
{
    clearQueue();
    while(!m_groups.isEmpty())
    {
        PlayListGroup *g = m_groups.takeFirst();
        g->d_ptr->m_trackList.clear();
        g->d_ptr->m_title0.clear();
        g->d_ptr->m_title1.clear();
        delete g;
    }
    m_lines.clear();
    return std::move(m_tracks);
}

void GroupedContainer::clear()
{
    clearQueue();
    while(!m_groups.isEmpty())
    {
        delete m_groups.takeFirst();
    }
    m_tracks.clear();
    m_lines.clear();
}

void GroupedContainer::reverseList()
{
    QList<PlayListTrack *> tracks = takeAllTracks();

    for (int i = 0; i < tracks.size() / 2 ;i++)
        tracks.swapItemsAt(i, tracks.size() - i - 1);

    addTracks(tracks);
}

void GroupedContainer::randomizeList()
{
    QRandomGenerator *rg = QRandomGenerator::global();
    QList<PlayListTrack *> tracks = takeAllTracks();

    for (int i = 0; i < tracks.size(); i++)
        tracks.swapItemsAt(rg->generate() % tracks.size(), rg->generate() % tracks.size());
    addTracks(tracks);
}

int GroupedContainer::lineCount() const
{
    updateCache();
    return m_lines.count();
}

PlayListItem *GroupedContainer::itemAtLine(int lineIndex) const
{
    updateCache();
    if (lineIndex < 0 || lineIndex >= m_lines.count())
        return nullptr;

    PlayListLine line = m_lines.at(lineIndex);
    if(line.isGroup)
        return m_groups.at(line.index);

    return m_tracks.at(line.index);
}

QList<PlayListItem *> GroupedContainer::itemsAtLines(int pos, int length) const
{
    updateCache();
    QList<PlayListLine> lines = m_lines.mid(pos, length);
    QList<PlayListItem *> out;
    for(const PlayListLine &line : std::as_const(lines))
    {
        if(line.isGroup)
            out << m_groups.at(line.index);
        else
            out << m_tracks.at(line.index);
    }

    return out;
}

int GroupedContainer::subIndexOfLine(int lineIndex) const
{
    updateCache();
    return m_lines[lineIndex].subindex;
}

int GroupedContainer::trackIndexAtLine(int lineIndex) const
{
    updateCache();
    if(lineIndex < 0 || lineIndex >= m_lines.count())
        return -1;

    return m_lines[lineIndex].isGroup ? -1 : m_lines[lineIndex].index;
}

bool GroupedContainer::alternateColor(int lineIndex) const
{
    updateCache();
    if(lineIndex < 0 || lineIndex >= m_lines.count())
        return false;

    return m_lines[lineIndex].alternateColor;
}

void GroupedContainer::updateCache() const
{
    if(!m_update)
        return;

    int lines = linesPerGroup();
    int t = 0;
    bool alternateColor = false;

    m_lines.clear();
    m_lines.reserve(lines * m_groups.count() + m_tracks.count());

    for(int g = 0; g < m_groups.count(); ++g)
    {

        for(int j = 0; j < lines; ++j)
        {
            PlayListLine line = {
                .isGroup = true,
                .index = g,
                .subindex = j,
                .alternateColor = alternateColor
            };
            m_lines << line;
        }

        alternateColor = !alternateColor;

        for(PlayListTrack *track : std::as_const(m_groups.at(g)->d_ptr->m_trackList))
        {
            PlayListLine line = {
                .isGroup = false,
                .index = t++,
                .subindex = 0,
                .alternateColor = alternateColor
            };
            track->d_ptr->trackIndex = line.index;
            m_lines << line;
            alternateColor = !alternateColor;
        }
    }
    m_update = false;
}
