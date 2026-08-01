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
#include <QFileInfo>
#include <QBuffer>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QImageReader>
#include <QCache>
#include <QRecursiveMutex>
#include "decoder.h"
#include "decoderfactory.h"
#include "abstractengine.h"
#include "inputsource.h"
#include "qmmpsettings.h"
#include "metadatamanager.h"

#define COVER_CACHE_SIZE 20

class MetaDataManagerPrivate
{
    Q_DECLARE_PUBLIC(MetaDataManager)
public:
    struct CoverCacheItem
    {
        QString coverPath;
        QImage coverImage;
    };

    MetaDataManagerPrivate(MetaDataManager *manager) :
        q_ptr(manager),
        coverCache(COVER_CACHE_SIZE)
    {}

private:
    QFileInfoList findCoverFiles(QDir dir, int depth) const
    {
        dir.setFilter(QDir::Files | QDir::Hidden);
        dir.setSorting(QDir::Name);
        QFileInfoList file_list = dir.entryInfoList(settings->coverNameFilters());

        const auto fileListCopy = file_list; //avoid container modification
        for(const QFileInfo &i : std::as_const(fileListCopy))
        {
            if(QDir::match(settings->coverNameFilters(false), i.fileName()))
                file_list.removeAll(i);

            if(QImageReader::imageFormat(i.filePath()).isEmpty()) //remove unsupported image formats
                file_list.removeAll(i);
        }
        if(!depth || !file_list.isEmpty())
            return file_list;

        depth--;
        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        dir.setSorting(QDir::Name);
        const QFileInfoList dir_info_list = dir.entryInfoList();
        for(const QFileInfo &i : std::as_const(dir_info_list))
        {
            file_list << findCoverFiles(QDir(i.absoluteFilePath()), depth);
        }
        return file_list;
    }

    CoverCacheItem *createCoverCacheItem(const QString &url) const
    {
        Q_Q(const MetaDataManager);

        CoverCacheItem *item = new CoverCacheItem;

        if(!url.contains(u"://"_s) && settings->useCoverFiles())
            item->coverPath = q->findCoverFile(url);

        if(item->coverPath.isEmpty())
        {
            MetaDataModel *model = q->createMetaDataModel(url, true);
            if(model)
            {
                item->coverPath = model->coverPath();
                item->coverImage = model->cover();
                delete model;
            }
        }

        if(!item->coverPath.isEmpty() && item->coverImage.isNull())
            item->coverImage = QImage(item->coverPath);

        if(item->coverImage.width() > 1024 || item->coverImage.height() > 1024)
            item->coverImage = item->coverImage.scaled(1024, 1024, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        return item;
    }

    static void destroy()
    {
        delete instance;
        instance = nullptr;
    }

    MetaDataManager *q_ptr;
    mutable QCache<QString, CoverCacheItem> coverCache;
    mutable QRecursiveMutex mutex;
    QmmpSettings *settings = QmmpSettings::instance();

    static MetaDataManager *instance;
};

MetaDataManager* MetaDataManagerPrivate::instance = nullptr;

MetaDataManager::MetaDataManager() : d_ptr(new MetaDataManagerPrivate(this))
{}

MetaDataManager::~MetaDataManager()
{
    delete d_ptr;
}

QList<TrackInfo> MetaDataManager::createPlayList(const QString &path, TrackInfo::Parts parts, QStringList *ignoredPaths) const
{
    QList<TrackInfo> list;
    DecoderFactory *fact = nullptr;
    EngineFactory *efact = nullptr;
    QStringList dummyList;
    if(!ignoredPaths)
        ignoredPaths = &dummyList;

    if(!path.contains(u"://"_s)) //local file
    {
        if(!QFile::exists(path))
            return list;

        if(!(fact = Decoder::findByFilePath(path, d_ptr->settings->determineFileTypeByContent())))
            efact = AbstractEngine::findByFilePath(path);
    }
    else
    {
        QString scheme = path.section(u"://"_s, 0, 0);
        if(InputSource::findByUrl(path))
        {
            list << TrackInfo(path);
        }
        else
        {
            for(DecoderFactory *f : Decoder::factories())
            {
                if(f->properties().protocols.contains(scheme) && Decoder::isEnabled(f))
                {
                    fact = f;
                    break;
                }
            }
        }
    }

    if(fact)
        list = fact->createPlayList(path, parts, ignoredPaths);
    else if(efact)
        list = efact->createPlayList(path, parts, ignoredPaths);

    for(TrackInfo &info : list)
    {
        if(info.value(Qmmp::DECODER).isEmpty() && (fact || efact))
            info.setValue(Qmmp::DECODER, fact ? fact->properties().shortName : efact->properties().shortName);
        if(info.value(Qmmp::FILE_SIZE).isEmpty() && !path.contains(u"://"_s))
            info.setValue(Qmmp::FILE_SIZE, QFileInfo(path).size());
    }
    return list;
}

MetaDataModel *MetaDataManager::createMetaDataModel(const QString &path, bool readOnly) const
{
    DecoderFactory *fact = nullptr;
    EngineFactory *efact = nullptr;

    if (!path.contains(u"://"_s)) //local file
    {
        if(!QFile::exists(path))
            return nullptr;
        if((fact = Decoder::findByFilePath(path, d_ptr->settings->determineFileTypeByContent())))
            return fact->createMetaDataModel(path, readOnly);
        if((efact = AbstractEngine::findByFilePath(path)))
            return efact->createMetaDataModel(path, readOnly);
        return nullptr;
    }

    QString scheme = path.section(u"://"_s, 0, 0);
    MetaDataModel *model = nullptr;
    if((fact = Decoder::findByProtocol(scheme)))
    {
        return fact->createMetaDataModel(path, readOnly);
    }
    for(EngineFactory *efact : AbstractEngine::enabledFactories())
    {
        if(efact->properties().protocols.contains(scheme))
            model = efact->createMetaDataModel(path, readOnly);
        if(model)
            return model;
    }
    return nullptr;
}

QStringList MetaDataManager::filters() const
{
    QStringList filters;
    for(const DecoderFactory *fact : Decoder::enabledFactories())
    {
        if (!fact->properties().filters.isEmpty())
            filters << QStringLiteral("%1 (%2)").arg(fact->properties().description, fact->properties().filters.join(QChar::Space));
    }
    for(const EngineFactory *fact : AbstractEngine::enabledFactories())
    {
        if (!fact->properties().filters.isEmpty())
            filters << QStringLiteral("%1 (%2)").arg(fact->properties().description, fact->properties().filters.join(QChar::Space));
    }
    return filters;
}

QStringList MetaDataManager::nameFilters() const
{
    QStringList filters = Decoder::nameFilters();
    filters << AbstractEngine::nameFilters();
    if(d_ptr->settings->determineFileTypeByContent())
        filters << u"*"_s;
    filters.removeDuplicates();
    return filters;
}

QStringList MetaDataManager::protocols() const
{
    QStringList p;
    p << InputSource::protocols();
    p << Decoder::protocols();
    p << AbstractEngine::protocols();
    p.removeDuplicates();
    return p;
}

QList<QRegularExpression> MetaDataManager::regExps() const
{
    return InputSource::regExps();
}

bool MetaDataManager::supports(const QString &fileName) const
{
    if (!fileName.contains(u"://"_s)) //local file
    {
        if(!QFile::exists(fileName))
            return false;
        if(Decoder::findByFilePath(fileName))
            return true;
        if(AbstractEngine::findByFilePath(fileName))
            return true;
        return false;
    }
    return false;
}

QImage MetaDataManager::getCover(const QString &url) const
{
    Q_D(const MetaDataManager);
    QMutexLocker locker(&d->mutex);
    MetaDataManagerPrivate::CoverCacheItem *item = d->coverCache.object(url);

    if(item)
        return item->coverImage;

    if(d->coverCache.insert(url, d->createCoverCacheItem(url)))
        return d->coverCache.object(url)->coverImage;

    return QImage();
}

QString MetaDataManager::getCoverPath(const QString &url) const
{
    Q_D(const MetaDataManager);
    QMutexLocker locker(&d->mutex);
    MetaDataManagerPrivate::CoverCacheItem *item = d->coverCache.object(url);

    if(item)
        return item->coverPath;

    if(d->coverCache.insert(url, d->createCoverCacheItem(url)))
        return d->coverCache.object(url)->coverPath;

    return QString();
}

QString MetaDataManager::findCoverFile(const QString &fileName) const
{
    Q_D(const MetaDataManager);
    if(!d->settings->useCoverFiles())
        return QString();

    if(!QFile::exists(fileName))
    {
        return QString();
    }

    QFileInfoList l = d->findCoverFiles(QFileInfo(fileName).absoluteDir(), d->settings->coverSearchDepth());
    return l.isEmpty() ? QString() : l.at(0).filePath();
}

void MetaDataManager::clearCoverCache()
{
    Q_D(MetaDataManager);
    QMutexLocker locker(&d->mutex);
    d->coverCache.clear();
}

void MetaDataManager::prepareForAnotherThread()
{
    //this hack should load all required plugins
    InputSource::enabledFactories();
    Decoder::enabledFactories();
    AbstractEngine::enabledFactories();
}

bool MetaDataManager::hasMatch(const QList<QRegularExpression> &regExps, const QString &path)
{
    for(const QRegularExpression &re : std::as_const(regExps))
    {
        if(re.match(path).hasMatch())
            return true;
    }
    return false;
}

MetaDataManager *MetaDataManager::instance()
{
    if(!MetaDataManagerPrivate::instance)
    {
        MetaDataManagerPrivate::instance = new MetaDataManager();
        qAddPostRoutine(&MetaDataManagerPrivate::destroy);
    }
    return MetaDataManagerPrivate::instance;
}
