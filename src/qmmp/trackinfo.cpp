/***************************************************************************
 *   Copyright (C) 2018-2026 by Ilya Kotov                                 *
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

#include <QRegularExpression>
#include "trackinfo.h"

class TrackInfoPrivate : public QSharedData
{
public:
    TrackInfoPrivate(const QString &path) : QSharedData(), path(path)
    {}

    void clear(TrackInfo::Parts parts)
    {
        if(parts & TrackInfo::MetaData)
            metaData.clear();
        if(parts & TrackInfo::Properties)
            properties.clear();
        if(parts & TrackInfo::ReplayGainInfo)
            replayGainInfo.clear();
        parts &= ~parts;
    }

    QMap<Qmmp::MetaData, QString> metaData;
    QMap<Qmmp::TrackProperty, QString> properties;
    QMap<Qmmp::ReplayGainKey, double> replayGainInfo;
    TrackInfo::Parts parts = TrackInfo::Parts();
    QString path;
    qint64 duration = 0;
};

TrackInfo::TrackInfo(const QString &path) : d(new TrackInfoPrivate(path))
{}

TrackInfo::TrackInfo(const TrackInfo &other) : d(other.d)
{}

TrackInfo::TrackInfo(TrackInfo &&other) noexcept
{
    swap(other);
}

TrackInfo::~TrackInfo()
{}

TrackInfo &TrackInfo::operator=(const TrackInfo &info)
{
    d = info.d;
    return *this;
}

TrackInfo &TrackInfo::operator=(TrackInfo &&info) noexcept
{
    swap(info);
    return *this;
}

bool TrackInfo::operator==(const TrackInfo &info) const
{
    return d->duration == info.duration() &&
            d->path == info.path() &&
            d->metaData == info.metaData () &&
            d->properties == info.properties() &&
            d->replayGainInfo == info.replayGainInfo() &&
           d->parts == info.parts();
}

bool TrackInfo::operator!=(const TrackInfo &info) const
{
    return !operator==(info);
}

qint64 TrackInfo::duration() const
{
    return d->duration;
}

bool TrackInfo::isEmpty() const
{
    return d->metaData.isEmpty() && d->properties.isEmpty() && d->replayGainInfo.isEmpty() && d->path.isEmpty();
}

QString TrackInfo::path() const
{
    return d->path;
}

QString TrackInfo::value(Qmmp::MetaData key) const
{
    return d->metaData.value(key);
}

QString TrackInfo::value(Qmmp::TrackProperty key) const
{
    return d->properties.value(key);
}

double TrackInfo::value(Qmmp::ReplayGainKey key) const
{
    return d->replayGainInfo.value(key);
}

QMap<Qmmp::MetaData, QString> TrackInfo::metaData() const
{
    return d->metaData;
}

QMap<Qmmp::TrackProperty, QString> TrackInfo::properties() const
{
    return d->properties;
}

QMap<Qmmp::ReplayGainKey, double> TrackInfo::replayGainInfo() const
{
    return d->replayGainInfo;
}

TrackInfo::Parts TrackInfo::parts() const
{
    return d->parts;
}

void TrackInfo::setDuration(qint64 duration)
{
    d->duration = duration;
}

void TrackInfo::setValue(Qmmp::MetaData key, const QVariant &value)
{
    QString strValue = value.toString().trimmed();
    if(strValue.isEmpty() || strValue == "0"_L1)
        d->metaData.remove(key);
    else
        d->metaData[key] = strValue;
    d->metaData.isEmpty() ? (d->parts &= ~MetaData) : (d->parts |= MetaData);
}

void TrackInfo::setValue(Qmmp::MetaData key, const char *value)
{
    setValue(key, QString::fromUtf8(value));
}

void TrackInfo::setValue(Qmmp::TrackProperty key, const QVariant &value)
{
    QString strValue = value.toString();
    if(strValue.isEmpty() || strValue == "0"_L1)
        d->properties.remove(key);
    else
        d->properties[key] = strValue;
    d->properties.isEmpty() ? (d->parts &= ~Properties) : (d->parts |= Properties);
}

void TrackInfo::setValue(Qmmp::TrackProperty key, const char *value)
{
    setValue(key, QString::fromUtf8(value));
}

void TrackInfo::setValue(Qmmp::ReplayGainKey key, double value)
{
    if(qFuzzyIsNull(value))
        d->replayGainInfo.remove(key);
    else
        d->replayGainInfo[key] = value;
    d->replayGainInfo.isEmpty() ? (d->parts &= ~ReplayGainInfo) : (d->parts |= ReplayGainInfo);
}

void TrackInfo::setValue(Qmmp::ReplayGainKey key, const QString &value)
{
    QString str = value;
    str.remove(QRegularExpression(u"[\\sA-Za-z]"_s));
    str = str.trimmed();
    bool ok = false;
    double v = str.toDouble(&ok);
    if(ok)
        setValue(key, v);
}

void TrackInfo::setValues(const QMap<Qmmp::MetaData, QString> &metaData)
{
    updateValues(metaData);
}

void TrackInfo::setValues(const QMap<Qmmp::TrackProperty, QString> &properties)
{
    updateValues(properties);
}

void TrackInfo::setValues(const QMap<Qmmp::ReplayGainKey, double> &replayGainInfo)
{
    updateValues(replayGainInfo);
}

void TrackInfo::updateValues(const QMap<Qmmp::MetaData, QString> &metaData)
{
    for(auto it = metaData.cbegin(); it != metaData.cend(); ++it)
        setValue(it.key(), it.value());
}

void TrackInfo::updateValues(const QMap<Qmmp::TrackProperty, QString> &properties)
{
    for(auto it = properties.cbegin(); it != properties.cend(); ++it)
        setValue(it.key(), it.value());
}

void TrackInfo::updateValues(const QMap<Qmmp::ReplayGainKey, double> &replayGainInfo)
{
    for(auto it = replayGainInfo.cbegin(); it != replayGainInfo.cend(); ++it)
        setValue(it.key(), it.value());
}

void TrackInfo::setPath(const QString &path)
{
    d->path = path;
}

void TrackInfo::clear(Parts parts)
{
    d->clear(parts);
}

void TrackInfo::clear()
{
    d->clear(AllParts);
    d->path.clear();
    d->duration = 0;
}

void TrackInfo::swap(TrackInfo &other)
{
    d.swap(other.d);
}

QString TrackInfo::pathFromUrl(const QString &url, int *track)
{
    if(track)
        *track = -1;

    int index1 = url.indexOf(u"://"_s);
    if(index1 < 0)
        return url;

    int index2 = url.lastIndexOf(QLatin1Char('#'));
    if(index2 < 0)
        return url;

    QString path = url.mid(index1 + 3, index2 - index1 - 3);
    QString trackStr = url.mid(index2 + 1);
    if(track)
        *track = trackStr.toInt();

    return path;
}
