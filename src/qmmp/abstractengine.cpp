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

#include <QSettings>
#include <QDir>
#include <QPluginLoader>
#include <QApplication>
#include "enginefactory.h"
#include "qmmpaudioengine_p.h"
#include "qmmpplugincache_p.h"
#include "qmmp.h"
#include "abstractengine.h"

Q_GLOBAL_STATIC(QList<QmmpPluginCache *>, engineCache);

class AbstractEnginePrivate
{
public:
    //sort cache items by priority
    static bool _pluginCacheLessComparator(const QmmpPluginCache* f1, const QmmpPluginCache* f2)
    {
        return f1->priority() < f2->priority();
    }

    static void loadPlugins()
    {
        if(engineCache.exists())
            return;

        QSettings settings;
        QVariantHash priorities = settings.value(u"Engine/priorities"_s).toHash();
        for(const QString &filePath : Qmmp::findPlugins(u"Engines"_s))
        {
            QmmpPluginCache *item = new QmmpPluginCache(filePath, &settings);
            if(item->hasError())
            {
                delete item;
                continue;
            }
            item->setPriority(priorities.value(item->shortName(), item->priority()).toInt());
            engineCache->append(item);
        }
        disabledNames = settings.value(u"Engine/disabled_plugins"_s).toStringList();
        std::stable_sort(engineCache->begin(), engineCache->end(), _pluginCacheLessComparator);
        QmmpPluginCache::cleanup(&settings);
        qAddPostRoutine(AbstractEnginePrivate::cleanup);
    }

    static void cleanup()
    {
        if(engineCache.exists())
        {
            qDeleteAll(*engineCache);
        }
    }

    QMutex mutex;
    static QStringList disabledNames;

};

QStringList AbstractEnginePrivate::disabledNames;

AbstractEngine::AbstractEngine(QObject *parent) :
    QThread(parent),
    d_ptr(new AbstractEnginePrivate)
{
}

AbstractEngine::~AbstractEngine()
{
    delete d_ptr;
}

QMutex *AbstractEngine::mutex()
{
    return &d_ptr->mutex;
}

// static methods

AbstractEngine *AbstractEngine::create(InputSource *s, QObject *parent)
{
    AbstractEngine *engine = new QmmpAudioEngine(parent); //internal engine
    if(!engine->enqueue(s))
    {
        engine->deleteLater();
        engine = nullptr;
    }
    else
        return engine;


    AbstractEnginePrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(AbstractEnginePrivate::disabledNames.contains(item->shortName()))
            continue;
        EngineFactory *fact = item->engineFactory();
        if(!fact)
            continue;
        engine = fact->create(parent); //engine plugin
        engine->setObjectName(item->shortName());
        if(!engine->enqueue(s))
        {
            engine->deleteLater();
            engine = nullptr;
        }
        else
            break;
    }
    return engine;
}

QList<EngineFactory *> AbstractEngine::factories()
{
    AbstractEnginePrivate::loadPlugins();
    QList<EngineFactory *> list;
    for(QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(item->engineFactory())
            list.append(item->engineFactory());
    }
    return list;
}

QList<EngineFactory *> AbstractEngine::enabledFactories()
{
    AbstractEnginePrivate::loadPlugins();
    QList<EngineFactory *> list;
    for(QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(AbstractEnginePrivate::disabledNames.contains(item->shortName()))
            continue;
        if(item->engineFactory())
            list.append(item->engineFactory());
    }
    return list;
}

QStringList AbstractEngine::nameFilters()
{
    AbstractEnginePrivate::loadPlugins();
    QStringList filters;
    for(QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(AbstractEnginePrivate::disabledNames.contains(item->shortName()))
            continue;

        filters << item->filters();
    }
    return filters;
}

QStringList AbstractEngine::contentTypes()
{
    AbstractEnginePrivate::loadPlugins();
    QStringList types;
    for(QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(AbstractEnginePrivate::disabledNames.contains(item->shortName()))
            continue;

        types << item->contentTypes();
    }
    return types;
}

EngineFactory *AbstractEngine::findByFilePath(const QString& source)
{
    AbstractEnginePrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(AbstractEnginePrivate::disabledNames.contains(item->shortName()))
            continue;
        EngineFactory *fact = item->engineFactory();
        if (fact && fact->supports(source))
            return fact;
    }
    return nullptr;
}

void AbstractEngine::setEnabled(EngineFactory *factory, bool enable)
{
    AbstractEnginePrivate::loadPlugins();
    if (!factories().contains(factory))
        return;

    if(enable == isEnabled(factory))
        return;

    if(enable)
        AbstractEnginePrivate::disabledNames.removeAll(factory->properties().shortName);
    else
        AbstractEnginePrivate::disabledNames.append(factory->properties().shortName);

    AbstractEnginePrivate::disabledNames.removeDuplicates();
    QSettings settings;
    settings.setValue(u"Engine/disabled_plugins"_s, AbstractEnginePrivate::disabledNames);
}

bool AbstractEngine::isEnabled(const EngineFactory *factory)
{
    AbstractEnginePrivate::loadPlugins();
    return !AbstractEnginePrivate::disabledNames.contains(factory->properties().shortName);
}

bool AbstractEngine::isEnabled(const AbstractEngine *engine)
{
    if(engine->objectName().isEmpty()) //qmmp engine
        return true;

    AbstractEnginePrivate::loadPlugins();
    return !AbstractEnginePrivate::disabledNames.contains(engine->objectName());
}

QString AbstractEngine::file(const EngineFactory *factory)
{
    AbstractEnginePrivate::loadPlugins();
    for(const QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(item->shortName() == factory->properties().shortName)
            return item->file();
    }
    return QString();
}

QStringList AbstractEngine::protocols()
{
    AbstractEnginePrivate::loadPlugins();
    QStringList protocolList;

    for(QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(AbstractEnginePrivate::disabledNames.contains(item->shortName()))
            continue;

         protocolList << item->protocols();
    }
    protocolList.removeDuplicates();
    return protocolList;
}

void AbstractEngine::setPriority(const EngineFactory *factory, int priority)
{
    AbstractEnginePrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(item->shortName() == factory->properties().shortName)
        {
            item->setPriority(priority);
            QSettings settings;
            QVariantHash priorities = settings.value(u"Engine/priorities"_s).toHash();
            priorities.insert(item->shortName(), priority);
            settings.setValue(u"Engine/priorities"_s, priorities);
            std::stable_sort(engineCache->begin(), engineCache->end(), AbstractEnginePrivate::_pluginCacheLessComparator);
            break;
        }
    }
}

int AbstractEngine::priority(const EngineFactory *factory)
{
    AbstractEnginePrivate::loadPlugins();
    for(const QmmpPluginCache *item : std::as_const(*engineCache))
    {
        if(item->shortName() == factory->properties().shortName)
            return item->priority();
    }
    return 0;
}
