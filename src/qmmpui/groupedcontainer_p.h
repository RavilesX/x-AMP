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

#ifndef GROUPEDCONTAINER_H
#define GROUPEDCONTAINER_H

#include <QList>
#include "playlistgroup.h"
#include "playlistcontainer_p.h"
#include "playlisttrack.h"

class GroupedContainer : public PlayListContainer
{
public:
    GroupedContainer();

    void addTrack(PlayListTrack *track) override;
    void addTracks(const QList<PlayListTrack *> &tracks) override;
    int insertTrack(int index, PlayListTrack *track) override;
    void replaceTracks(const QList<PlayListTrack *> &tracks) override;
    QList<PlayListGroup *> groups() const override;
    QList<PlayListTrack *> tracks() const override;
    int groupCount() const override;
    int trackCount() const override;
    QList<PlayListTrack *> mid(int pos, int count) const override;
    bool isEmpty() const override;
    void clearSelection() override;
    int indexOf(PlayListItem *item) const override;
    PlayListTrack *track(int index) const override;
    PlayListGroup *group(int index) const override;
    bool contains(PlayListTrack *track) const override;
    void removeTrack(PlayListTrack *track) override;
    void removeTracks(QList<PlayListTrack *> tracks) override;
    bool move(const QList<int> &indexes, int from, int to) override;
    QList<PlayListTrack *> takeAllTracks() override;
    void clear() override;
    void reverseList() override;
    void randomizeList() override;

    //playlist view api
    int lineCount() const override;
    PlayListItem *itemAtLine(int lineIndex) const override;
    QList<PlayListItem *> itemsAtLines(int pos, int length = -1) const override;
    int subIndexOfLine(int lineIndex) const override;
    int trackIndexAtLine(int lineIndex) const override;
    bool alternateColor(int lineIndex) const override;

private:
    void updateCache() const;

    struct PlayListLine
    {
      bool isGroup = false;
      int index = -1;
      int subindex = -1;
      bool alternateColor = false;
    };

    QList<PlayListTrack *> m_tracks;
    QList<PlayListGroup *> m_groups;
    mutable QList<PlayListLine> m_lines;
    mutable bool m_update = true;
};

#endif // GROUPEDCONTAINER_H
