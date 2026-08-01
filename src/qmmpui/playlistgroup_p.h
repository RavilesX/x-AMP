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

#ifndef PLAYLISTGROUP_P_H
#define PLAYLISTGROUP_P_H

#include <QString>
#include <QImage>

class GroupedContainer;
class QmmpUiSettings;
class MetaDataHelper;
class PlayListTrack;

class PlayListGroupPrivate
{
public:
    PlayListGroupPrivate(const QString &name);
    ~PlayListGroupPrivate();

    QString formatTitle0() const;
    QString formatTitle1() const;

    QList<PlayListTrack *> m_trackList; //A list of tracks
    mutable QString m_title0;
    mutable QString m_title1;
    QString m_groupName;

    bool m_isCoverLoaded = false;
    QImage m_cover;

private:
    QmmpUiSettings *m_settings;
    MetaDataHelper *m_helper;
};

#endif //PLAYLISTGROUP_P_H
