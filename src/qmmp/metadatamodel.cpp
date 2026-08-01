/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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

#include "metadatamodel.h"

class MetaDataItemPrivate
{
public:
    QString name, suffix;
    QVariant value;
};

MetaDataItem::MetaDataItem(const QString &name, const QVariant &value, const QString &suffix) :
    d_ptr(new MetaDataItemPrivate)
{
    Q_D(MetaDataItem);
    d->name = name;
    d->value = value;
    d->suffix = suffix;
}

MetaDataItem::MetaDataItem(const MetaDataItem &other) :
    d_ptr(new MetaDataItemPrivate)
{
    operator = (other);
}

MetaDataItem::MetaDataItem(MetaDataItem &&other) noexcept :
    d_ptr(std::exchange(other.d_ptr, nullptr))
{}

MetaDataItem::~MetaDataItem()
{
    delete d_ptr;
}

MetaDataItem &MetaDataItem::operator=(const MetaDataItem &other)
{
    Q_D(MetaDataItem);
    d->name = other.d_ptr->name;
    d->value = other.d_ptr->value;
    d->suffix = other.d_ptr->suffix;
    return *this;
}

MetaDataItem &MetaDataItem::operator=(MetaDataItem &&other) noexcept
{
    std::swap(d_ptr, other.d_ptr);
    return *this;
}

QString MetaDataItem::name() const
{
    return d_ptr->name;
}

void MetaDataItem::setName(const QString &name)
{
    d_ptr->name = name;
}

QVariant MetaDataItem::value() const
{
    return d_ptr->value;
}

void MetaDataItem::setValue(const QVariant &value)
{
    d_ptr->value = value;
}

QString MetaDataItem::suffix() const
{
    return d_ptr->suffix;
}

void MetaDataItem::setSuffix(const QString &suffix)
{
    d_ptr->suffix = suffix;
}

class MetaDataModelPrivate
{
public:
    MetaDataModelPrivate(bool ro, MetaDataModel::DialogHints hints) :
        readOnly(ro),
        dialogHints(hints)
    {}

    bool readOnly;
    MetaDataModel::DialogHints dialogHints;
};

MetaDataModel::MetaDataModel(bool readOnly, DialogHints hints) :
    d_ptr(new MetaDataModelPrivate(readOnly, hints))
{}

MetaDataModel::~MetaDataModel()
{
    delete d_ptr;
}

QList<MetaDataItem> MetaDataModel::extraProperties() const
{
    return QList<MetaDataItem>();
}

QList<MetaDataItem> MetaDataModel::descriptions() const
{
    return QList<MetaDataItem>();
}

QList<TagModel *> MetaDataModel::tags() const
{
    return QList<TagModel *>();
}

QImage MetaDataModel::cover() const
{
    return QImage();
}

void MetaDataModel::setCover(const QImage &img)
{
    Q_UNUSED(img);
}

void MetaDataModel::removeCover()
{}

QString MetaDataModel::coverPath() const
{
    return QString();
}

QString MetaDataModel::cue() const
{
    return QString();
}

void MetaDataModel::setCue(const QString &content)
{
    Q_UNUSED(content);
}

void MetaDataModel::removeCue()
{}

QString MetaDataModel::lyrics() const
{
    return QString();
}

void MetaDataModel::setLyrics(const QString &content)
{
     Q_UNUSED(content);
}

void MetaDataModel::removeLyrics()
{
    setLyrics(QString());
}

bool MetaDataModel::isReadOnly() const
{
    return d_ptr->readOnly;
}

MetaDataModel::DialogHints MetaDataModel::dialogHints() const
{
    return d_ptr->dialogHints;
}

void MetaDataModel::setDialogHints(const DialogHints &hints)
{
    d_ptr->dialogHints = hints;
}

void MetaDataModel::setReadOnly(bool readOnly)
{
    d_ptr->readOnly = readOnly;
}
