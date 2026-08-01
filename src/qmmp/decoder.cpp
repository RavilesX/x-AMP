// Copyright (c) 2000-2001 Brad Hughes <bhughes@trolltech.com>
//
// Use, modification and distribution is allowed without limitation,
// warranty, or liability of any kind.
//
#include <QStringList>
#include <QSettings>
#include <QBuffer>
#include <QDir>
#include <math.h>
#include <algorithm>
#include "qmmpplugincache_p.h"
#include "decoderfactory.h"
#include "decoder.h"

Q_GLOBAL_STATIC(QList<QmmpPluginCache *>, decoderCache);

class DecoderPrivate
{
public:
    DecoderPrivate(QIODevice *input) : input(input) {}

    //sort cache items by priority
    static bool _pluginCacheLessComparator(const QmmpPluginCache* f1, const QmmpPluginCache* f2)
    {
        return f1->priority() < f2->priority();
    }

    static void loadPlugins()
    {
        if(decoderCache.exists())
            return;

        QSettings settings;
        QVariantHash priorities = settings.value(u"Decoder/priorities"_s).toHash();
        for(const QString &filePath : Qmmp::findPlugins(u"Input"_s))
        {
            QmmpPluginCache *item = new QmmpPluginCache(filePath, &settings);
            if(item->hasError())
            {
                delete item;
                continue;
            }
            item->setPriority(priorities.value(item->shortName(), item->priority()).toInt());
            decoderCache->append(item);
        }
        disabledNames = settings.value(u"Decoder/disabled_plugins"_s).toStringList();
        std::stable_sort(decoderCache->begin(), decoderCache->end(), _pluginCacheLessComparator);
        QmmpPluginCache::cleanup(&settings);
        qAddPostRoutine(DecoderPrivate::cleanup);
    }

    static void cleanup()
    {
        if(decoderCache.exists())
        {
            QSettings settings;
            for(QmmpPluginCache *item : std::as_const(*decoderCache))
            {
                item->update(&settings);
                delete item;
            }
        }
    }

    static QStringList disabledNames;
    AudioParameters parameters;
    QMap<Qmmp::TrackProperty, QString> properties;
    QIODevice *input;
    bool hasMetaData = false;
    QMap<Qmmp::MetaData, QString> metaData;
    QMap<Qmmp::ReplayGainKey, double> m_rg; //replay gain information
};

QStringList DecoderPrivate::disabledNames;

Decoder::Decoder(QIODevice *input) : d_ptr(new DecoderPrivate(input))
{}

Decoder::~Decoder()
{
    delete d_ptr;
}

void Decoder::setReplayGainInfo(const QMap<Qmmp::ReplayGainKey, double> &rg)
{
    d_ptr->m_rg = rg;
}

void Decoder::configure(quint32 srate, const ChannelMap &map, Qmmp::AudioFormat format)
{
    configure(AudioParameters(srate, map, format));
}

void Decoder::configure(quint32 srate, int channels, Qmmp::AudioFormat f)
{
    qCDebug(core) << "using internal channel order";
    configure(AudioParameters(srate, ChannelMap(channels), f));
}

void Decoder::configure(const AudioParameters &p)
{
    Q_D(Decoder);
    d->parameters = p;
    setProperty(Qmmp::SAMPLERATE, d->parameters.sampleRate());
    setProperty(Qmmp::CHANNELS, d->parameters.channels());
    setProperty(Qmmp::BITS_PER_SAMPLE, d->parameters.validBitsPerSample());
}

void Decoder::next()
{}

QString Decoder::nextURL() const
{
    return QString();
}

AudioParameters Decoder::audioParameters() const
{
    return d_ptr->parameters;
}

QMap<Qmmp::ReplayGainKey, double> Decoder::replayGainInfo() const
{
    return d_ptr->m_rg;
}

void Decoder::addMetaData(const QMap<Qmmp::MetaData, QString> &metaData)
{
    Q_D(Decoder);
    d->metaData = metaData;
    d->hasMetaData = true;
}

QIODevice *Decoder::input() const
{
    return d_ptr->input;
}

bool Decoder::hasMetaData() const
{
    return d_ptr->hasMetaData;
}

QMap<Qmmp::MetaData, QString> Decoder::takeMetaData()
{
    Q_D(Decoder);
    d->hasMetaData = false;
    return d->metaData;
}

void Decoder::setProperty(Qmmp::TrackProperty key, const QVariant &value)
{
    Q_D(Decoder);
    QString strValue = value.toString();
    if(strValue.isEmpty() || strValue == "0"_L1)
        d->properties.remove(key);
    else
        d->properties[key] = strValue;
}

void Decoder::setProperties(const QMap<Qmmp::TrackProperty, QString> &properties)
{
    for(auto it = properties.cbegin(); it != properties.cend(); ++it)
        setProperty(it.key(), it.value());
}

QMap<Qmmp::TrackProperty, QString> Decoder::properties() const
{
    return d_ptr->properties;
}

// static methods
QString Decoder::file(const DecoderFactory *factory)
{
    DecoderPrivate::loadPlugins();
    for(const QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(item->shortName() == factory->properties().shortName)
            return item->file();
    }
    return QString();
}

QStringList Decoder::protocols()
{
    DecoderPrivate::loadPlugins();
    QStringList protocolList;

    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(DecoderPrivate::disabledNames.contains(item->shortName()))
            continue;

        protocolList << item->protocols();
    }
    protocolList.removeDuplicates();
    return protocolList;
}

DecoderFactory *Decoder::findByFilePath(const QString &path, bool useContent)
{
    DecoderPrivate::loadPlugins();

    //get list of available/supported factories
    QList<DecoderFactory*> filtered = useContent ? enabledFactories() : findByFileExtension(path);

    if(filtered.isEmpty())
        return nullptr;

    //try to find by content
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        qCWarning(core, "file open error: %s", qPrintable(file.errorString()));
        return nullptr;
    }

    for(DecoderFactory *fact : std::as_const(filtered))
    {
        if(fact->canDecode(&file))
            return fact;
    }

    //fallback: try to find by extension
    if(useContent)
        filtered = findByFileExtension(path);

    for(DecoderFactory *fact : std::as_const(filtered))
    {
        if(fact->properties().noInput || fact->properties().protocols.contains(u"file"_s))
            return fact;
    }

    //fallback: try to find by content
    if(!useContent)
        return findByContent(&file);

    //fallback: use first available factory
    if(!filtered.isEmpty() && !useContent)
        return filtered.constFirst();

    return nullptr;
}

DecoderFactory *Decoder::findByMime(const QString& type)
{
    if(type.isEmpty())
        return nullptr;
    DecoderPrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(DecoderPrivate::disabledNames.contains(item->shortName()))
            continue;
        DecoderFactory *fact = item->decoderFactory();
        if(fact && !fact->properties().noInput && fact->properties().contentTypes.contains(type))
            return fact;
    }
    return nullptr;
}

DecoderFactory *Decoder::findByContent(QIODevice *input)
{
    DecoderPrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(DecoderPrivate::disabledNames.contains(item->shortName()))
            continue;
        DecoderFactory *fact = item->decoderFactory();
        if(fact && !fact->properties().noInput && fact->canDecode(input))
            return fact;
    }
    return nullptr;
}

DecoderFactory *Decoder::findByProtocol(const QString &p)
{
    DecoderPrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(DecoderPrivate::disabledNames.contains(item->shortName()))
            continue;

        if(item->decoderFactory() && item->decoderFactory()->properties().protocols.contains(p))
            return item->decoderFactory();
    }
    return nullptr;
}

QList<DecoderFactory *> Decoder::findByFileExtension(const QString &path)
{
    QList<DecoderFactory*> filtered;
    DecoderFactory *fact = nullptr;
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(DecoderPrivate::disabledNames.contains(item->shortName()))
            continue;

        if(!(fact = item->decoderFactory()))
            continue;

        if(QDir::match(fact->properties().filters, path.section(QLatin1Char('/'), -1)))
            filtered.append(fact);
    }

    return filtered;
}

void Decoder::setEnabled(DecoderFactory *factory, bool enable)
{
    DecoderPrivate::loadPlugins();
    if (!factories().contains(factory))
        return;

    if(enable == isEnabled(factory))
        return;

    if(enable)
        DecoderPrivate::disabledNames.removeAll(factory->properties().shortName);
    else
        DecoderPrivate::disabledNames.append(factory->properties().shortName);

    DecoderPrivate::disabledNames.removeDuplicates();
    QSettings settings;
    settings.setValue(u"Decoder/disabled_plugins"_s, DecoderPrivate::disabledNames);
}

bool Decoder::isEnabled(const DecoderFactory *factory)
{
    DecoderPrivate::loadPlugins();
    return !DecoderPrivate::disabledNames.contains(factory->properties().shortName);
}

void Decoder::setPriority(const DecoderFactory *factory, int priority)
{
    DecoderPrivate::loadPlugins();
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(item->shortName() == factory->properties().shortName)
        {
            item->setPriority(priority);
            QSettings settings;
            QVariantHash priorities = settings.value(u"Decoder/priorities"_s).toHash();
            priorities.insert(item->shortName(), priority);
            settings.setValue(u"Decoder/priorities"_s, priorities);
            std::stable_sort(decoderCache->begin(), decoderCache->end(), DecoderPrivate::_pluginCacheLessComparator);
            break;
        }
    }
}

int Decoder::priority(const DecoderFactory *factory)
{
    DecoderPrivate::loadPlugins();
    for(const QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(item->shortName() == factory->properties().shortName)
            return item->priority();
    }
    return 0;
}

QList<DecoderFactory *> Decoder::factories()
{
    DecoderPrivate::loadPlugins();
    QList<DecoderFactory *> list;
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(item->decoderFactory())
            list.append(item->decoderFactory());
    }
    return list;
}

QList<DecoderFactory *> Decoder::enabledFactories()
{
    DecoderPrivate::loadPlugins();
    QList<DecoderFactory *> list;
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(DecoderPrivate::disabledNames.contains(item->shortName()))
            continue;
        if(item->decoderFactory())
            list.append(item->decoderFactory());
    }
    return list;
}

QStringList Decoder::nameFilters()
{
    DecoderPrivate::loadPlugins();
    QStringList filters;
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(DecoderPrivate::disabledNames.contains(item->shortName()))
            continue;

        filters << item->filters();
    }
    return filters;
}

QStringList Decoder::contentTypes()
{
    DecoderPrivate::loadPlugins();
    QStringList types;
    for(QmmpPluginCache *item : std::as_const(*decoderCache))
    {
        if(DecoderPrivate::disabledNames.contains(item->shortName()))
            continue;

        types << item->contentTypes();
    }
    return types;
}
