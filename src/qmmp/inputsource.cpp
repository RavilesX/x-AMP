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

#include <QFile>
#include <QDir>
#include <algorithm>
#include "qcoreapplication.h"
#include "qmmpplugincache_p.h"
#include "qmmp.h"
#include "fileinputsource_p.h"
#include "emptyinputsource_p.h"
#include "inputsource.h"

Q_GLOBAL_STATIC(QList<QmmpPluginCache *>, inputCache);

class InputSourcePrivate
{
public:
    InputSourcePrivate(const QString &source) : path(source) {}

    static void loadPlugins()
    {
        if(inputCache.exists())
            return;

        QSettings settings;
        for(const QString &filePath : Qmmp::findPlugins(u"Transports"_s))
        {
            QmmpPluginCache *item = new QmmpPluginCache(filePath, &settings);
            if(item->hasError())
            {
                delete item;
                continue;
            }
            inputCache->append(item);
        }
        disabledNames = settings.value(u"Transports/disabled_plugins"_s).toStringList();
        QmmpPluginCache::cleanup(&settings);
        qAddPostRoutine(InputSourcePrivate::cleanup);
    }

    static void cleanup()
    {
        if(inputCache.exists())
        {
            qDeleteAll(*inputCache);
        }
    }

    QString path;
    qint64 offset = -1;
    QMap<Qmmp::MetaData, QString> metaData;
    QMap<Qmmp::TrackProperty, QString> properties;
    QHash<QString, QString> streamInfo;
    bool m_hasMetaData = false, hasStreamInfo = false;
    static QStringList disabledNames;
};

QStringList InputSourcePrivate::disabledNames;

InputSource::InputSource(const QString &source, QObject *parent) :
    QObject(parent),
    d_ptr(new InputSourcePrivate(source))
{}

InputSource::~InputSource()
{
    delete d_ptr;
}

bool InputSource::isWaiting() const
{
    return false;
}

QString InputSource::contentType() const
{
    return QString();
}

void InputSource::stop()
{}

const QString InputSource::path() const
{
    return d_ptr->path;
}

qint64 InputSource::offset() const
{
    return d_ptr->offset;
}
void InputSource::setOffset(qint64 offset)
{
    d_ptr->offset = offset;
}

bool InputSource::hasMetaData() const
{
    return d_ptr->m_hasMetaData;
}

QMap<Qmmp::MetaData, QString> InputSource::takeMetaData()
{
    d_ptr->m_hasMetaData = false;
    return d_ptr->metaData;
}

void InputSource::setProperty(Qmmp::TrackProperty key, const QVariant &value)
{
    QString strValue = value.toString();
    if(strValue.isEmpty() || strValue == "0"_L1)
        d_ptr->properties.remove(key);
    else
        d_ptr->properties[key] = strValue;
}

void InputSource::setProperties(const QMap<Qmmp::TrackProperty, QString> &properties)
{
    for(auto it = properties.cbegin(); it != properties.cend(); ++it)
        setProperty(it.key(), it.value());
}

const QMap<Qmmp::TrackProperty, QString> &InputSource::properties() const
{
    return d_ptr->properties;
}

void InputSource::addMetaData(const QMap<Qmmp::MetaData, QString> &metaData)
{
    d_ptr->metaData = metaData;
    d_ptr->m_hasMetaData = true;
}

void InputSource::addStreamInfo(const QHash<QString, QString> &info)
{
    d_ptr->streamInfo = info;
    d_ptr->hasStreamInfo = true;
}

bool InputSource::hasStreamInfo() const
{
    return d_ptr->hasStreamInfo;
}

QHash<QString, QString> InputSource::takeStreamInfo()
{
    d_ptr->hasStreamInfo = false;
    return d_ptr->streamInfo;
}

// static methods
InputSource *InputSource::create(const QString &url, QObject *parent)
{
    InputSourcePrivate::loadPlugins();
    if(!url.contains(u"://"_s)) //local file path doesn't contain "://"
    {
        qCDebug(core) << "using file transport";
        return new FileInputSource(url, parent);
    }

    InputSourceFactory *factory = findByUrl(url);

    if(factory)
    {
        qCDebug(core, "using %s transport", qPrintable(url.section(u"://"_s, 0, 0)));
        return factory->create(url, parent);
    }

    qCDebug(core) << "using fake transport";
    return new EmptyInputSource(url, parent);
}

QList<InputSourceFactory *> InputSource::factories()
{
    InputSourcePrivate::loadPlugins();
    QList<InputSourceFactory *> list;
    for(QmmpPluginCache *item : std::as_const(*inputCache))
    {
        if(item->inputSourceFactory())
            list.append(item->inputSourceFactory());
    }
    return list;
}

QList<InputSourceFactory *> InputSource::enabledFactories()
{
    InputSourcePrivate::loadPlugins();
    QList<InputSourceFactory *> list;
    for(QmmpPluginCache *item : std::as_const(*inputCache))
    {
        if(InputSourcePrivate::disabledNames.contains(item->shortName()))
            continue;
        if(item->inputSourceFactory())
            list.append(item->inputSourceFactory());
    }
    return list;
}

QString InputSource::file(const InputSourceFactory *factory)
{
    InputSourcePrivate::loadPlugins();
    auto it = std::find_if(inputCache->cbegin(), inputCache->cend(),
                           [factory](QmmpPluginCache *item) { return item->shortName() == factory->properties().shortName; } );
    return it == inputCache->cend() ? QString() : (*it)->file();
}

QStringList InputSource::protocols()
{
    InputSourcePrivate::loadPlugins();
    QStringList protocolList;

    for(QmmpPluginCache *item : std::as_const(*inputCache))
    {
        if(InputSourcePrivate::disabledNames.contains(item->shortName()))
            continue;

        protocolList << item->protocols();
    }
    protocolList.removeDuplicates();
    return protocolList;
}

QList<QRegularExpression> InputSource::regExps()
{
    InputSourcePrivate::loadPlugins();
    QList<QRegularExpression> regExpList;

    for(QmmpPluginCache *item : std::as_const(*inputCache))
    {
        if(InputSourcePrivate::disabledNames.contains(item->shortName()))
            continue;
        if(item->inputSourceFactory())
            regExpList << item->inputSourceFactory()->properties().regExps;
    }
    return regExpList;
}

InputSourceFactory *InputSource::findByUrl(const QString &url)
{
    InputSourcePrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*inputCache))
    {
        if(InputSourcePrivate::disabledNames.contains(item->shortName()))
            continue;

        InputSourceFactory *factory = item->inputSourceFactory();
        if(!factory)
            continue;

        for(const QRegularExpression &r : factory->properties().regExps)
        {
            if(r.match(url).hasMatch())
                return factory;
        }
    }

    for(QmmpPluginCache *item : std::as_const(*inputCache))
    {
        if(InputSourcePrivate::disabledNames.contains(item->shortName()))
            continue;

        InputSourceFactory *factory = item->inputSourceFactory();

        if(factory && factory->properties().protocols.contains(url.section(u"://"_s, 0, 0)))
            return factory;
    }

    return nullptr;
}

void InputSource::setEnabled(InputSourceFactory *factory, bool enable)
{
    InputSourcePrivate::loadPlugins();
    if(!factories().contains(factory))
        return;

    if(enable == isEnabled(factory))
        return;

    if(enable)
        InputSourcePrivate::disabledNames.removeAll(factory->properties().shortName);
    else
        InputSourcePrivate::disabledNames.append(factory->properties().shortName);

    InputSourcePrivate::disabledNames.removeDuplicates();
    QSettings settings;
    settings.setValue("Transports/disabled_plugins"_L1, InputSourcePrivate::disabledNames);
}

bool InputSource::isEnabled(const InputSourceFactory *factory)
{
    InputSourcePrivate::loadPlugins();
    return !InputSourcePrivate::disabledNames.contains(factory->properties().shortName);
}
