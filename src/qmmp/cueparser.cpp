/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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
#include <QTextStream>
#include "cueparser.h"

class CueParserPrivate
{
public:
    QStringList splitLine(const QString &line)
    {
        QStringList list;
        QString buf = line.trimmed();
        if(buf.isEmpty())
            return list;

        while(!buf.isEmpty())
        {
            if(buf.startsWith(QLatin1Char('"')))
            {
                int end = buf.indexOf(QLatin1Char('"'), 1);
                if(end == -1) //ignore invalid line
                {
                    list.clear();
                    qCWarning(core, "unable to parse line: %s",qPrintable(line));
                    return list;
                }
                list << buf.mid (1, end - 1);
                buf.remove (0, end + 1);
            }
            else
            {
                int end = buf.indexOf(QChar::Space, 0);
                if(end < 0)
                    end = buf.size();
                list << buf.mid (0, end);
                buf.remove (0, end);
            }
            buf = buf.trimmed();
        }
        return list;
    }

    qint64 getLength(const QString &str)
    {
        QStringList list = str.split(QLatin1Char(':'));
        if(list.size() == 2)
            return (qint64)list.at(0).toInt() * 60000 + list.at(1).toInt() * 1000;
        if(list.size() == 3)
            return (qint64)list.at(0).toInt() * 60000 + list.at(1).toInt() * 1000 + list.at(2).toInt() * 1000 / 75;
        return 0;
    }

    struct CUETrack
    {
        TrackInfo info;
        QString file;
        qint64 offset = 0;
    };
    QList<CUETrack *> m_tracks;
    QStringList m_files;
};

CueParser::CueParser() : d_ptr(new CueParserPrivate)
{}

CueParser::CueParser(const QByteArray &data, const QByteArray &codecName) : d_ptr(new CueParserPrivate)
{
    loadData(data, codecName);
}

CueParser::CueParser(const char *data, qsizetype size, const QByteArray &codecName) : CueParser(QByteArray(data, size), codecName)
{}

CueParser::~CueParser()
{
    clear();
    delete d_ptr;
}

void CueParser::loadData(const QByteArray &data, const QByteArray &codecName)
{
    QmmpTextCodec codec(codecName);
    loadData(data, &codec);
}

void CueParser::loadData(const QByteArray &data, QmmpTextCodec *codec)
{
    Q_D(CueParser);
    clear();

    if(data.isEmpty())
        return;

    QString artist, album, genre, date, comment, file;
    double album_peak = 0.0, album_gain = 0.0;
    QString str = codec->toUnicode(data);
    QTextStream textStream(&str, QIODeviceBase::ReadOnly);

    while (!textStream.atEnd())
    {
        QString line = textStream.readLine().trimmed();
        QStringList words = d->splitLine(line);
        if(words.size() < 2)
            continue;

        if(words[0] == "FILE"_L1)
        {
            file = words[1];
            d->m_files << file;
        }
        else if(words[0] == "PERFORMER"_L1)
        {
            if(d->m_tracks.isEmpty())
                artist = words[1];
            else
                d->m_tracks.last()->info.setValue(Qmmp::ARTIST, words[1]);
        }
        else if(words[0] == "TITLE"_L1)
        {
            if(d->m_tracks.isEmpty())
                album = words[1];
            else
                d->m_tracks.last()->info.setValue(Qmmp::TITLE, words[1]);
        }
        else if(words[0] == "TRACK"_L1)
        {
            TrackInfo info;
            info.setValue(Qmmp::TRACK, words[1].toInt());
            info.setValue(Qmmp::ALBUM, album);
            info.setValue(Qmmp::GENRE, genre);
            info.setValue(Qmmp::YEAR, date);
            info.setValue(Qmmp::COMMENT, comment);
            info.setValue(Qmmp::ARTIST, artist);
            info.setValue(Qmmp::ALBUMARTIST, artist);
            info.setValue(Qmmp::REPLAYGAIN_ALBUM_GAIN, album_gain);
            info.setValue(Qmmp::REPLAYGAIN_ALBUM_PEAK, album_peak);

            d->m_tracks << new CueParserPrivate::CUETrack;
            d->m_tracks.last()->info = info;
            d->m_tracks.last()->offset = 0;
        }
        else if(words[0] == "INDEX"_L1 && words[1] == "01"_L1)
        {
            if(d->m_tracks.isEmpty())
                continue;
            d->m_tracks.last()->offset = d->getLength(words[2]);
            d->m_tracks.last()->file = file;
        }
        else if(words[0] == "REM"_L1)
        {
            if(words.size() < 3)
                continue;
            if(words[1] == "GENRE"_L1)
            {
                if(d->m_tracks.isEmpty())
                    genre = words[2];
                else
                    d->m_tracks.last()->info.setValue(Qmmp::GENRE, words[2]);
            }
            else if(words[1] == "DATE"_L1)
            {
                 if(d->m_tracks.isEmpty())
                     date = words[2];
                 else
                     d->m_tracks.last()->info.setValue(Qmmp::YEAR, words[2]);
            }
            else if(words[1] == "COMMENT"_L1)
            {
                 if(d->m_tracks.isEmpty())
                     comment = words[2];
                 else
                     d->m_tracks.last()->info.setValue(Qmmp::COMMENT, words[2]);
            }
            else if(words[1] == "REPLAYGAIN_ALBUM_GAIN"_L1)
                album_gain = words[2].toDouble();
            else if(words[1] == "REPLAYGAIN_ALBUM_PEAK"_L1)
                album_peak = words[2].toDouble();
            else if(words[1] == "REPLAYGAIN_TRACK_GAIN"_L1 && !d->m_tracks.isEmpty())
                d->m_tracks.last()->info.setValue(Qmmp::REPLAYGAIN_TRACK_GAIN, words[2].toDouble());
            else if(words[1] == "REPLAYGAIN_TRACK_PEAK"_L1 && !d->m_tracks.isEmpty())
                d->m_tracks.last()->info.setValue(Qmmp::REPLAYGAIN_TRACK_PEAK, words[2].toDouble());
        }
    }
    if(d->m_tracks.isEmpty())
        qCWarning(core, "invalid cue data");
}

QList<TrackInfo> CueParser::createPlayList(int track) const
{
    Q_D(const CueParser);
    QList<TrackInfo> out;

    if(track <= 0)
    {
        for(const CueParserPrivate::CUETrack *track : std::as_const(d->m_tracks))
            out << TrackInfo(track->info);
    }
    else if(track > d->m_tracks.count())
    {
        qCWarning(core, "invalid track number: %d", track);
        return out;
    }
    else
    {
        out << TrackInfo(d->m_tracks.at(track - 1)->info);
    }

    return out;
}

const QStringList &CueParser::files() const
{
    return d_ptr->m_files;
}

qint64 CueParser::offset(int track) const
{
    if(track < 1 || track > d_ptr->m_tracks.count())
    {
        qCWarning(core, "invalid track number: %d", track);
        return 0;
    }
    return d_ptr->m_tracks.at(track - 1)->offset;
}

qint64 CueParser::duration(int track) const
{
    if(track < 1 || track > d_ptr->m_tracks.count())
    {
        qCWarning(core, "invalid track number: %d", track);
        return 0;
    }
    return d_ptr->m_tracks.at(track - 1)->info.duration();
}

QString CueParser::file(int track) const
{
    if(track < 1 || track > d_ptr->m_tracks.count())
    {
        qCWarning(core, "invalid track number: %d", track);
        return QString();
    }
    return d_ptr->m_tracks.at(track - 1)->file;
}

QString CueParser::url(int track) const
{
    if(track < 1 || track > d_ptr->m_tracks.count())
    {
        qCWarning(core, "invalid track number: %d", track);
        return QString();
    }
    return d_ptr->m_tracks.at(track - 1)->info.path();
}

int CueParser::count() const
{
    return d_ptr->m_tracks.count();
}

bool CueParser::isEmpty() const
{
    return d_ptr->m_tracks.isEmpty();
}

TrackInfo CueParser::info(int track) const
{
    if(track < 1 || track > d_ptr->m_tracks.count())
    {
        qCWarning(core, "invalid track number: %d", track);
        return TrackInfo();
    }
    return d_ptr->m_tracks.at(track - 1)->info;
}

void CueParser::setDuration(const QString &file, qint64 duration)
{
    Q_D(CueParser);
    for(int i = 0; i < d_ptr->m_tracks.count(); ++i)
    {
        CueParserPrivate::CUETrack *track = d->m_tracks.at(i);
        if(track->file == file)
        {
            if((i == d->m_tracks.count() - 1) || (d->m_tracks.at(i + 1)->file != track->file))
                track->info.setDuration(duration - track->offset);
            else
                track->info.setDuration(d->m_tracks.at(i + 1)->offset - track->offset);

            if(track->info.duration() < 0)
                track->info.setDuration(0);
        }
    }
}

void CueParser::setDuration(qint64 duration)
{
    Q_D(CueParser);
    for(int i = 0; i < d->m_tracks.count(); ++i)
    {
        CueParserPrivate::CUETrack *track = d->m_tracks.at(i);

        if(i == d->m_tracks.count() - 1)
            track->info.setDuration(duration - track->offset);
        else
            track->info.setDuration(d->m_tracks.at(i + 1)->offset - track->offset);

        if(track->info.duration() < 0)
            track->info.setDuration(0);
    }
}

void CueParser::setProperties(const QString &file, const QMap<Qmmp::TrackProperty, QString> &properties)
{
    Q_D(CueParser);
    for(CueParserPrivate::CUETrack *track : std::as_const(d->m_tracks))
    {
        if(track->file == file)
            track->info.setValues(properties);
    }
}

void CueParser::setProperties(const QMap<Qmmp::TrackProperty, QString> &properties)
{
    Q_D(CueParser);
    for(CueParserPrivate::CUETrack *track : std::as_const(d->m_tracks))
        track->info.setValues(properties);
}

void CueParser::setMetaData(int track, Qmmp::MetaData key, const QVariant &value)
{
    Q_D(CueParser);
    if(track < 1 || track > d->m_tracks.count())
        qCWarning(core, "invalid track number: %d", track);

    d->m_tracks.at(track - 1)->info.setValue(key, value);
}

void CueParser::setUrl(const QString &scheme, const QString &path)
{
    Q_D(CueParser);
    for(int i = 0; i < d->m_tracks.count(); ++i)
        d->m_tracks.at(i)->info.setPath(QStringLiteral("%1://%2#%3").arg(scheme, path, d->m_tracks.at(i)->info.value(Qmmp::TRACK)));
}

void CueParser::clear()
{
    Q_D(CueParser);
    qDeleteAll(d->m_tracks);
    d->m_tracks.clear();
    d->m_files.clear();
}
