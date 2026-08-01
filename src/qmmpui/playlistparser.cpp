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

#include <QPluginLoader>
#include <QList>
#include <QDir>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <qmmp/qmmp.h>
#include "playlistformat.h"
#include "playlistparser.h"

Q_GLOBAL_STATIC(QList<PlayListFormat*>, plFormats)

class PlayListParserPrivate
{
public:
    static const QHash<QString, Qmmp::MetaData> metaKeys;
    static const QHash<QString, Qmmp::TrackProperty> propKeys;
};

//key names
const QHash<QString, Qmmp::MetaData> PlayListParserPrivate::metaKeys = {
    { u"title"_s, Qmmp::TITLE },
    { u"artist"_s, Qmmp::ARTIST },
    { u"albumArtist"_s, Qmmp::ALBUMARTIST },
    { u"album"_s, Qmmp::ALBUM },
    { u"comment"_s, Qmmp::COMMENT },
    { u"genre"_s, Qmmp::GENRE },
    { u"composer"_s, Qmmp::COMPOSER },
    { u"year"_s, Qmmp::YEAR },
    { u"track"_s, Qmmp::TRACK },
    { u"disk"_s, Qmmp::DISCNUMBER }
};

const QHash<QString, Qmmp::TrackProperty>  PlayListParserPrivate::propKeys = {
    { u"bitrate"_s, Qmmp::BITRATE },
    { u"samplerate"_s, Qmmp::SAMPLERATE },
    { u"channels"_s, Qmmp::CHANNELS },
    { u"bitsPerSample"_s, Qmmp::BITS_PER_SAMPLE },
    { u"formatName"_s, Qmmp::FORMAT_NAME },
    { u"decoder"_s, Qmmp::DECODER },
    { u"fileSize"_s, Qmmp::FILE_SIZE }
};

QList<PlayListFormat *> PlayListParser::formats()
{
    loadFormats();
    return *plFormats;
}

QStringList PlayListParser::nameFilters()
{
    loadFormats();
    QStringList filters;
    for(const PlayListFormat *format : std::as_const(*plFormats))
    {
        filters << format->properties().filters;
    }
    return filters;
}

QStringList PlayListParser::filters()
{
    loadFormats();
    QStringList filters;
    for(const PlayListFormat *format : std::as_const(*plFormats))
    {
        if (!format->properties().filters.isEmpty())
            filters << QStringLiteral("%1 (%2)").arg(format->properties().shortName.toUpper(),  format->properties().filters.join(QChar::Space));
    }
    return filters;
}

bool PlayListParser::isPlayList(const QString &url)
{
    return QDir::match(nameFilters(), url.section(QLatin1Char('/'), -1));
}

PlayListFormat *PlayListParser::findByMime(const QString &mime)
{
    loadFormats();
    auto it = std::find_if(plFormats->cbegin(), plFormats->cend(),
                           [mime](PlayListFormat *format) { return format->properties().contentTypes.contains(mime); } );
    return it == plFormats->cend() ? nullptr : *it;
}

PlayListFormat *PlayListParser::findByPath(const QString &filePath)
{
    loadFormats();
    for(PlayListFormat *format : std::as_const(*plFormats))
    {
        if(QDir::match(format->properties().filters, filePath.section(QLatin1Char('/'), -1)))
            return format;
    }
    return nullptr;
}

PlayListFormat *PlayListParser::findByUrl(const QUrl &url)
{
    QString path = url.path(QUrl::FullyEncoded);
    return findByPath(path);
}

void PlayListParser::savePlayList(QList<PlayListTrack *> tracks, const QString &f_name)
{
    if(tracks.isEmpty())
        return;
    PlayListFormat* prs = PlayListParser::findByPath(f_name);
    if (!prs)
        return;
    QFile file(f_name);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(prs->encode(tracks, QFileInfo(f_name).canonicalFilePath()));
        file.close();
    }
    else
        qCWarning(core, "unable to save playlist, error: %s", qPrintable(file.errorString()));
}

QList<PlayListTrack *> PlayListParser::loadPlaylist(const QString &f_name)
{
    if(!QFile::exists(f_name))
        return QList<PlayListTrack *>();
    PlayListFormat* prs = PlayListParser::findByPath(f_name);
    if(!prs)
        return QList<PlayListTrack *>();

    QFile file(f_name);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCWarning(core, "unable to open playlist, error: %s", qPrintable(file.errorString()));
        return QList<PlayListTrack *>();
    }

    const QList<PlayListTrack*> tracks = prs->decode(file.readAll());

    if(tracks.isEmpty())
    {
        qCWarning(core, "error opening %s",qPrintable(f_name));
        return tracks;
    }

    QString path;
    for(PlayListTrack *t : std::as_const(tracks))
    {
        path = t->path();

        if(path.contains(u"://"_s))
            continue;

        if(QFileInfo(path).isRelative())
            path.prepend(QFileInfo(f_name).canonicalPath() + QLatin1Char('/'));

        path.replace(QLatin1Char('\\'), QLatin1Char('/'));
        path.replace(u"//"_s, u"/"_s);
        t->setPath(path);
    }
    return tracks;
}

QList<PlayListTrack *> PlayListParser::loadPlaylist(const QString &fmt, const QByteArray &content)
{
    auto it = std::find_if(plFormats->cbegin(), plFormats->cend(),
                           [fmt](PlayListFormat *format) { return format->properties().shortName == fmt; } );
    return it == plFormats->cend() ? QList<PlayListTrack *>() : (*it)->decode(content);
}

void PlayListParser::loadFormats()
{
    if(plFormats.exists())
        return;

    for(const QString &filePath : Qmmp::findPlugins(u"PlayListFormats"_s))
    {
        QPluginLoader loader(filePath);
        QObject *plugin = loader.instance();
        if (loader.isLoaded())
            qCDebug(core) << "loaded plugin" << QFileInfo(filePath).filePath();
        else
            qCWarning(core) << loader.errorString();

        PlayListFormat *fmt = nullptr;
        if(plugin)
            fmt = qobject_cast<PlayListFormat *>(plugin);

        if(fmt)
            plFormats->append(fmt);
    }
}

QByteArray PlayListParser::serialize(const QList<PlayListTrack *> &tracks)
{
    QJsonArray array;
    for(const PlayListTrack *t : std::as_const(tracks))
    {
        QJsonObject obj;
        QString value;
        for(QHash<QString, Qmmp::MetaData>::const_iterator it = PlayListParserPrivate::metaKeys.constBegin();
             it != PlayListParserPrivate::metaKeys.constEnd(); ++it)
        {
            if(!(value = t->value(it.value())).isEmpty())
                obj.insert(it.key(), value);
        }

        for(QHash<QString, Qmmp::TrackProperty>::const_iterator it = PlayListParserPrivate::propKeys.constBegin();
             it != PlayListParserPrivate::propKeys.constEnd(); ++it)
        {
            if(!(value = t->value(it.value())).isEmpty())
                obj.insert(it.key(), value);
        }

        obj.insert(u"path"_s, t->path());
        obj.insert(u"duration"_s, t->duration());
        array.append(obj);
    }

    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

QList<PlayListTrack *> PlayListParser::deserialize(const QByteArray &json)
{
    QList<PlayListTrack *> out;

    QJsonDocument document = QJsonDocument::fromJson(json);
    if(!document.isArray())
    {
        qCWarning(core, "invalid JSON array");
        return out;
    }

    QJsonArray array = document.array();
    for(QJsonArray::const_iterator it = array.constBegin(); it != array.constEnd(); ++it)
    {
        if(!(*it).isObject())
            continue;

        QJsonObject obj = (*it).toObject();

        if(obj.value(u"path"_s).isNull())
            continue;

        PlayListTrack *t = new PlayListTrack();
        t->setPath(obj.value(u"path"_s).toString());
        t->setDuration(obj.value(u"duration"_s).toDouble());

        Qmmp::MetaData metaKey;
        Qmmp::TrackProperty propKey;

        for(QJsonObject::const_iterator i = obj.constBegin(); i != obj.constEnd(); ++i)
        {
            if((metaKey = PlayListParserPrivate::metaKeys.value(i.key(), Qmmp::UNKNOWN)) != Qmmp::UNKNOWN)
                t->setValue(metaKey, i.value().toString());
            else if((propKey = PlayListParserPrivate::propKeys.value(i.key(), Qmmp::UNKNOWN_PROPERTY)) != Qmmp::UNKNOWN_PROPERTY)
                t->setValue(propKey, i.value().toString());
        }

        out << t;
    }

    return out;
}
