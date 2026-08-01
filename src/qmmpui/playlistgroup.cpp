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

#include <QApplication>
#include "qmmpuisettings.h"
#include "metadatahelper_p.h"
#include "playlistgroup_p.h"
#include "playlistgroup.h"

PlayListGroup::PlayListGroup(const QString &groupName) :
    d_ptr(new PlayListGroupPrivate(groupName))
{}

PlayListGroup::~PlayListGroup()
{
    delete d_ptr;
}

QString PlayListGroup::formattedTitle(int line) const
{
    Q_D(const PlayListGroup);
    if(line == 0)
    {
        if(d->m_title0.isEmpty())
            d->m_title0 = d->formatTitle0();

        return d->m_title0;
    }

    if(line == 1)
    {
        if(d->m_title1.isEmpty())
            d->m_title1 = d->formatTitle1();

        return d->m_title1;
    }

    return QString();
}

QStringList PlayListGroup::formattedTitles() const
{
    Q_D(const PlayListGroup);
    if(d->m_title0.isEmpty())
        d->m_title0 = d->formatTitle0();

    if(d->m_title1.isEmpty())
        d->m_title1 = d->formatTitle1();

    return { d->m_title0, d->m_title1 };
}

bool PlayListGroup::contains(const PlayListTrack *track) const
{
    return d_ptr->m_trackList.contains(track);
}

bool PlayListGroup::isEmpty() const
{
    return d_ptr->m_trackList.isEmpty();
}

QList<PlayListTrack *> PlayListGroup::tracks() const
{
    return d_ptr->m_trackList;
}

int PlayListGroup::count() const
{
    return d_ptr->m_trackList.count();
}

QString PlayListGroup::formattedDuration() const
{
    return QString();
}

QString PlayListGroup::groupName() const
{
    return d_ptr->m_groupName;
}

bool PlayListGroup::isGroup() const
{
    return true;
}

QString PlayListGroup::firstTrackPath() const
{
    Q_D(const PlayListGroup);
    return d->m_trackList.isEmpty() ? QString() : d->m_trackList.constFirst()->path();
}

bool PlayListGroup::isCoverLoaded() const
{
    return d_ptr->m_isCoverLoaded;
}

QImage PlayListGroup::cover() const
{
    return d_ptr->m_cover;
}

void PlayListGroup::setCover(const QImage &cover)
{
    Q_D(PlayListGroup);
    d->m_isCoverLoaded = true;
    d->m_cover = cover;
}

void PlayListGroup::updateMetaData()
{
    Q_D(PlayListGroup);
    d->m_title0.clear();
    d->m_title1.clear();
}

PlayListGroupPrivate::PlayListGroupPrivate(const QString &name) :
    m_groupName(name),
    m_settings(QmmpUiSettings::instance()),
    m_helper(MetaDataHelper::instance())
{}

PlayListGroupPrivate::~PlayListGroupPrivate()
{
    while(!m_trackList.isEmpty())
    {
        PlayListTrack *mf = m_trackList.takeFirst();

        if (mf->isUsed())
            mf->deleteLater();
        else
            delete mf;
    }
}

QString PlayListGroupPrivate::formatTitle0() const
{
    if(m_trackList.isEmpty())
        return QString();

    if(m_trackList.constFirst()->path().contains(u"://"_s) && !m_trackList.constFirst()->path().contains(QLatin1Char('#')))
        return QCoreApplication::translate("PlayListGroup", "Streams");

    qint64 duration = 0;
    for(const PlayListTrack *t : std::as_const(m_trackList))
        duration += t->duration();

    TrackInfo info = *m_trackList.constFirst();
    info.setDuration(duration);

    QString title = m_helper->groupFormatter0()->format(info);
    if(m_settings->convertUnderscore())
        title.replace(QLatin1Char('_'), QChar::Space);
    if(m_settings->convertTwenty())
        title.replace(u"%20"_s, u" "_s);

    if(title.isEmpty())
        return QCoreApplication::translate("PlayListGroup", "Empty group");

    return title;
}

QString PlayListGroupPrivate::formatTitle1() const
{
    if(m_trackList.isEmpty() || m_trackList.constFirst()->properties().isEmpty())
        return QString();

    qint64 duration = 0;
    for(const PlayListTrack *t : std::as_const(m_trackList))
        duration += t->duration();

    TrackInfo info = *m_trackList.constFirst();
    info.setDuration(duration);

    QString title = m_helper->groupFormatter1()->format(info);
    if(m_settings->convertUnderscore())
        title.replace(QLatin1Char('_'), QChar::Space);
    if(m_settings->convertTwenty())
        title.replace(u"%20"_s, u" "_s);

    return title;
}


