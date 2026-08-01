/***************************************************************************
 *   Copyright (C) 2007-2026 by Ilya Kotov                                 *
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

#include <algorithm>
#include <QStringList>
#include <QDir>
#include "qmmpaudioengine_p.h"
#include "qmmp.h"
#include "qmmpplugincache_p.h"
#include "effectfactory.h"
#include "effect.h"

Q_GLOBAL_STATIC(QList<QmmpPluginCache *>, effectCache);

class EffectPrivate
{
public:
    static bool _effectCacheCompareFunc(QmmpPluginCache *e1, QmmpPluginCache *e2)
    {
        return e1->priority() > e2->priority();
    }

    static void loadPlugins()
    {
        if(effectCache.exists())
            return;

        QSettings settings;
        for(const QString &filePath : Qmmp::findPlugins(u"Effect"_s))
        {
            QmmpPluginCache *item = new QmmpPluginCache(filePath, &settings);
            if(item->hasError())
            {
                delete item;
                continue;
            }
            effectCache->append(item);
        }

        std::stable_sort(effectCache->begin(), effectCache->end(), _effectCacheCompareFunc);
        enabledNames = settings.value(u"Effect/enabled_plugins"_s).toStringList();
        qAddPostRoutine(EffectPrivate::cleanup);
    }

    static void cleanup()
    {
        if(effectCache.exists())
        {
            qDeleteAll(*effectCache);
        }
    }

    EffectFactory *factory = nullptr;
    quint32 freq = 0;
    int channels = 0;
    ChannelMap m_chan_map;

    static QStringList enabledNames;
};

QStringList EffectPrivate::enabledNames;

Effect::Effect() : d_ptr(new EffectPrivate)
{}

Effect::~Effect()
{
    delete d_ptr;
}

void Effect::configure(quint32 freq, ChannelMap map)
{
    Q_D(Effect);
    d->freq = freq;
    d->m_chan_map = map;
    d->channels = map.count();
}

quint32 Effect::sampleRate() const
{
    return d_ptr->freq;
}

int Effect::channels() const
{
    return d_ptr->channels;
}

ChannelMap Effect::channelMap() const
{
    return d_ptr->m_chan_map;
}

AudioParameters Effect::audioParameters() const
{
    Q_D(const Effect);
    return AudioParameters(d->freq, d->m_chan_map, Qmmp::PCM_FLOAT);
}

EffectFactory *Effect::factory() const
{
    return d_ptr->factory;
}

//static members
Effect *Effect::create(EffectFactory *factory)
{
    EffectPrivate::loadPlugins();
    Effect *effect = factory->create();
    effect->d_ptr->factory = factory;
    return effect;
}

QList<EffectFactory *> Effect::factories()
{
    EffectPrivate::loadPlugins();
    QList<EffectFactory *> list;
    for(QmmpPluginCache *item : std::as_const(*effectCache))
    {
        if(item->effectFactory())
            list.append(item->effectFactory());
    }
    return list;
}

QList<EffectFactory *> Effect::enabledFactories()
{
    EffectPrivate::loadPlugins();
    QList<EffectFactory *> list;
    for(QmmpPluginCache *item : std::as_const(*effectCache))
    {
        if(EffectPrivate::enabledNames.contains(item->shortName()) && item->effectFactory())
            list.append(item->effectFactory());
    }
    return list;
}

QString Effect::file(const EffectFactory *factory)
{
    EffectPrivate::loadPlugins();
    for(const QmmpPluginCache *item : std::as_const(*effectCache))
    {
        if(item->shortName() == factory->properties().shortName)
            return item->file();
    }
    return QString();
}

void Effect::setEnabled(EffectFactory *factory, bool enable)
{
    EffectPrivate::loadPlugins();
    if (!factories().contains(factory))
        return;

    if(enable == isEnabled(factory))
        return;

    if(enable)
    {
        if(QmmpAudioEngine::instance())
            QmmpAudioEngine::instance()->addEffect(factory);
        EffectPrivate::enabledNames.append(factory->properties().shortName);
    }
    else
    {
        EffectPrivate::enabledNames.removeAll(factory->properties().shortName);
        if(QmmpAudioEngine::instance())
            QmmpAudioEngine::instance()->removeEffect(factory);
    }

    EffectPrivate::enabledNames.removeDuplicates();

    QSettings settings;
    settings.setValue(u"Effect/enabled_plugins"_s, EffectPrivate::enabledNames);
    QmmpPluginCache::cleanup(&settings);
}

bool Effect::isEnabled(const EffectFactory *factory)
{
    EffectPrivate::loadPlugins();
    return EffectPrivate::enabledNames.contains(factory->properties().shortName);
}

EffectFactory *Effect::findFactory(const QString &shortName)
{
    EffectPrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*effectCache))
    {
        if(item->shortName() == shortName)
            return item->effectFactory();
    }
    return nullptr;
}
