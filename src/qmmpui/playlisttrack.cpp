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

#include <array>
#include <qmmp/metadatamanager.h>
#include "qmmpuisettings.h"
#include "metadatahelper_p.h"
#include "playlisttrack.h"
#include "playlisttrack_p.h"

PlayListTrack::PlayListTrack() :
    TrackInfo(),
    PlayListItem(),
    d_ptr(new PlayListTrackPrivate(this))
{}

PlayListTrack::PlayListTrack(const PlayListTrack &other) :
    TrackInfo(other),
    PlayListItem(),
    d_ptr(new PlayListTrackPrivate(this))
{
    Q_D(PlayListTrack);
    d->formattedTitles = other.d_ptr->formattedTitles;
    d->group = other.d_ptr->group;
    d->formattedLength = other.d_ptr->formattedLength;
    d->titleFormats = other.d_ptr->titleFormats;
    d->groupFormat = other.d_ptr->groupFormat;
    d->formattedLength = other.d_ptr->formattedLength;
    setSelected(other.isSelected());
}

PlayListTrack::PlayListTrack(const PlayListTrack *other) :
    PlayListTrack(*other)
{}

PlayListTrack::PlayListTrack(PlayListTrack &&other) noexcept
{
    TrackInfo::swap(other);
    std::swap(d_ptr, other.d_ptr);
}

PlayListTrack::PlayListTrack(const TrackInfo &info) :
    TrackInfo(info),
    PlayListItem(),
    d_ptr(new PlayListTrackPrivate(this))
{}

PlayListTrack::PlayListTrack(TrackInfo &&info) :
    PlayListItem(),
    d_ptr(new PlayListTrackPrivate(this))
{
    TrackInfo::swap(info);
}

PlayListTrack::~PlayListTrack()
{
    if(d_ptr->refCount != 0)
        qCWarning(core, "deleting busy track");
    delete d_ptr;
}

PlayListTrack &PlayListTrack::operator=(const PlayListTrack &other)
{
    Q_D(PlayListTrack);
    TrackInfo::operator = (other);
    d->formattedTitles = other.d_ptr->formattedTitles;
    d->group = other.d_ptr->group;
    d->formattedLength = other.d_ptr->formattedLength;
    d->titleFormats = other.d_ptr->titleFormats;
    d->groupFormat = other.d_ptr->groupFormat;
    d->formattedLength = other.d_ptr->formattedLength;
    setSelected(other.isSelected());
    return *this;
}

PlayListTrack &PlayListTrack::operator=(PlayListTrack &&other) noexcept
{
    TrackInfo::swap(other);
    std::swap(d_ptr, other.d_ptr);
    return *this;
}

void PlayListTrack::updateMetaData(const TrackInfo &info)
{
    Q_D(PlayListTrack);
    setValues(info.metaData());
    if(info.parts() & TrackInfo::Properties)
        setValues(info.properties());
    if(info.parts() & TrackInfo::ReplayGainInfo)
        setValues(info.replayGainInfo());
    setDuration(info.duration());
    setPath(info.path());
    d->formattedTitles.clear();
    d->formattedLength.clear();
    d->group.clear();
    d->formatGroup();
}

void PlayListTrack::updateMetaData()
{
    QList<TrackInfo> list = MetaDataManager::instance()->createPlayList(path());
    if(list.count() == 1 && list.constFirst().path() == path() && list.constFirst().parts() != TrackInfo::Parts())
    {
        updateMetaData(list.constFirst());
    }
}

QString PlayListTrack::groupName() const
{
    Q_D(const PlayListTrack);
    QString groupFormat = d->settings->groupFormat();
    if(d->settings->groupExtraRowVisible())
        groupFormat.append(d->settings->groupExtraRowFormat());

    if(d->group.isEmpty() || d->groupFormat != groupFormat)
    {
        d->groupFormat = groupFormat;
        d->formatGroup();
    }
    return d->group;
}

bool PlayListTrack::isGroup() const
{
    return false;
}

int PlayListTrack::trackIndex() const
{
    return d_ptr->trackIndex;
}

int PlayListTrack::queuedIndex() const
{
    return d_ptr->queuedIndex;
}

bool PlayListTrack::isQueued() const
{
    return d_ptr->queuedIndex >= 0;
}

void PlayListTrack::beginUsage()
{
    d_ptr->refCount++;
}

void PlayListTrack::endUsage()
{
    d_ptr->refCount--;
}

void PlayListTrack::deleteLater()
{
    d_ptr->sheduledForDeletion = true;
}

bool PlayListTrack::isSheduledForDeletion() const
{
    return d_ptr->sheduledForDeletion;
}

bool PlayListTrack::isUsed() const
{
    return (d_ptr->refCount != 0);
}

QString PlayListTrack::formattedTitle(int column) const
{
    Q_D(const PlayListTrack);
    if(d->formattedTitles.count() != d->helper->columnCount())
    {
        while(d->formattedTitles.count() > d->helper->columnCount())
            d->formattedTitles.takeLast();

        while(d->formattedTitles.count() < d->helper->columnCount())
            d->formattedTitles.append(QString());

        while(d->titleFormats.count() > d->helper->columnCount())
            d->titleFormats.takeLast();

        while(d->titleFormats.count() < d->helper->columnCount())
            d->titleFormats.append(QString());
    }

    if(column < 0 || column >= d->formattedTitles.size())
    {
        qCWarning(core, "column number is out of range");
        return QString();
    }

    if(d->formattedTitles[column].isEmpty() || d->titleFormats[column] != d->helper->titleFormatter(column)->pattern())
    {
        d->titleFormats[column] = d->helper->titleFormatter(column)->pattern();
        d->formatTitle(column);
    }
    return d->formattedTitles[column];
}

QStringList PlayListTrack::formattedTitles() const
{
    Q_D(const PlayListTrack);
    if(d->formattedTitles.count() != d->helper->columnCount())
    {
        while(d->formattedTitles.count() > d->helper->columnCount())
            d->formattedTitles.takeLast();

        while(d->formattedTitles.count() < d->helper->columnCount())
            d->formattedTitles.append(QString());

        while(d->titleFormats.count() > d->helper->columnCount())
            d->titleFormats.takeLast();

        while(d->titleFormats.count() < d->helper->columnCount())
            d->titleFormats.append(QString());
    }

    for(int column = 0; column < d->helper->columnCount(); column++)
    {
        if(d->formattedTitles[column].isEmpty() ||
                d->titleFormats[column] != d->helper->titleFormatter(column)->pattern() ||
                d->titleFormats[column].contains(u"%I"_s))
        {
            d->titleFormats[column] = d->helper->titleFormatter(column)->pattern();
            d->formatTitle(column);
        }
    }

    return d->formattedTitles;
}

QString PlayListTrack::formattedDuration() const
{
    Q_D(const PlayListTrack);
    if(duration() > 0 && d->formattedLength.isEmpty())
        d->formattedLength = MetaDataFormatter::formatDuration(duration());
    else if(duration() <= 0 && !d->formattedLength.isEmpty())
        d->formattedLength.clear();
    return d->formattedLength;
}

PlayListTrackPrivate::PlayListTrackPrivate(PlayListTrack *track) :
    q_ptr(track),
    settings(QmmpUiSettings::instance()),
    helper(MetaDataHelper::instance())

{}

void PlayListTrackPrivate::formatTitle(int column) const
{
    Q_Q(const PlayListTrack);
    formattedTitles[column] = helper->titleFormatter(column)->format(q);
    if(formattedTitles.count() == 1)
    {
        if (formattedTitles[column].isEmpty())
            formattedTitles[column] = q->path().section(QLatin1Char('/'), -1);
        if (formattedTitles[column].isEmpty())
            formattedTitles[column] = q->path();
    }
    if(settings->convertUnderscore())
        formattedTitles[column].replace(QLatin1Char('_'), QChar::Space);
    if(settings->convertTwenty())
        formattedTitles[column].replace(u"%20"_s, u" "_s);
}

void PlayListTrackPrivate::formatGroup() const
{
    Q_Q(const PlayListTrack);
    if(q->path().contains(u"://"_s) && !q->path().contains(QLatin1Char('#')))
    {
        group = QStringLiteral("Streams");
        return;
    }

    //suitable for grouping
    static const std::array<Qmmp::MetaData, 8> groupingKeys = {
        Qmmp::ARTIST,
        Qmmp::ALBUMARTIST,
        Qmmp::ALBUM,
        Qmmp::COMMENT,
        Qmmp::GENRE,
        Qmmp::COMPOSER,
        Qmmp::YEAR,
        Qmmp::DISCNUMBER
    };

    PlayListTrack track;
    track.setPath(q->path());

    for(Qmmp::MetaData key : std::as_const(groupingKeys))
        track.setValue(key, q->value(key));

    group = helper->groupFormatter0()->format(track);

    if(settings->groupExtraRowVisible())
        group += helper->groupFormatter1()->format(track);

    if (group.isEmpty())
        group = QStringLiteral("Empty group");
}
