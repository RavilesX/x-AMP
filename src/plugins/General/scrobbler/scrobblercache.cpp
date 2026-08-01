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

#include <QFile>
#include "scrobblercache.h"

SongInfo::SongInfo()
{}

SongInfo::SongInfo(const TrackInfo &info) : TrackInfo(info)
{}

SongInfo::SongInfo(const SongInfo &other) : TrackInfo(other),
    m_start_ts(other.timeStamp())
{}

SongInfo::~SongInfo()
{}

SongInfo & SongInfo::operator=(const SongInfo &info)
{
    TrackInfo::operator=(info);
    m_start_ts = info.timeStamp();
    return *this;
}

bool SongInfo::operator==(const SongInfo &info)
{
    return (TrackInfo::operator==(info)) && (m_start_ts == info.timeStamp());
}

bool SongInfo::operator!=(const SongInfo &info)
{
    return !operator==(info);
}

void SongInfo::setTimeStamp(uint ts)
{
    m_start_ts = ts;
}

uint SongInfo::timeStamp() const
{
    return m_start_ts;
}

ListenCache::ListenCache(const QString &filePath) : m_filePath(filePath)
{}

QList<SongInfo> ListenCache::load()
{
    QList<SongInfo> songs;
    QFile file(m_filePath);

    if(!file.open(QIODevice::ReadOnly))
        return QList<SongInfo>();

    while(!file.atEnd())
    {
        int s;
        QString line = QString::fromUtf8(file.readLine()).trimmed();
        if((s = line.indexOf(QLatin1Char('='))) < 0)
            continue;

        QString param = line.left(s);
        QString value = line.right(line.size() - s - 1);

        if(param == "title"_L1)
        {
            songs << SongInfo();
            songs.last().setValue(Qmmp::TITLE, value);
        }
        else if (songs.isEmpty())
            continue;
        else if (param == "artist"_L1)
            songs.last().setValue(Qmmp::ARTIST, value);
        else if (param == "album"_L1)
            songs.last().setValue(Qmmp::ALBUM, value);
        else if (param == "comment"_L1)
            songs.last().setValue(Qmmp::COMMENT, value);
        else if (param == "genre"_L1)
            songs.last().setValue(Qmmp::GENRE, value);
        else if (param == "year"_L1)
            songs.last().setValue(Qmmp::YEAR, value);
        else if (param == "track"_L1)
            songs.last().setValue(Qmmp::TRACK, value);
        else if (param == "length"_L1) //1.3.x config support
            songs.last().setDuration(value.toInt() * 1000);
        else if (param == "duration"_L1)
            songs.last().setDuration(value.toLongLong());
        else if (param == "time"_L1)
            songs.last().setTimeStamp(value.toUInt());
    }
    file.close();
    return songs;
}

void ListenCache::save(const QList<SongInfo> &songs)
{
    QFile file(m_filePath);
    if (songs.isEmpty())
    {
        file.remove();
        return;
    }
    if(!file.open(QIODevice::WriteOnly))
    {
        qCWarning(plugin, "unable to save file %s", qPrintable(m_filePath));
        qCWarning(plugin, "error %d: %s", file.error(), qPrintable(file.errorString()));
        return;
    }
    for(const SongInfo &m : std::as_const(songs))
    {
        file.write(QStringLiteral("title=%1\n").arg(m.value(Qmmp::TITLE)).toUtf8());
        file.write(QStringLiteral("artist=%1\n").arg(m.value(Qmmp::ARTIST)).toUtf8());
        file.write(QStringLiteral("album=%1\n").arg(m.value(Qmmp::ALBUM)).toUtf8());
        file.write(QStringLiteral("comment=%1\n").arg(m.value(Qmmp::COMMENT)).toUtf8());
        file.write(QStringLiteral("genre=%1\n").arg(m.value(Qmmp::GENRE)).toUtf8());
        file.write(QStringLiteral("year=%1\n").arg(m.value(Qmmp::YEAR)).toUtf8());
        file.write(QStringLiteral("track=%1\n").arg(m.value(Qmmp::TRACK)).toUtf8());
        file.write(QStringLiteral("duration=%1\n").arg(m.duration()).toLatin1());
        file.write(QStringLiteral("time=%1\n").arg(m.timeStamp()).toLatin1());
    }
    file.close();
}
