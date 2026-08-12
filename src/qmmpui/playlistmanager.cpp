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

#include <QByteArray>
#include <QFile>
#include <QBuffer>
#include <QDir>
#include <QTimer>
#include <QSettings>
#include <QSaveFile>
#include <QMap>
#include "qcoreapplication.h"
#include "playlistqueue_p.h"
#include "qmmpuisettings.h"
#include "playlistmanager.h"

using namespace Qt::Literals::StringLiterals;

class PlayListManagerPrivate
{
    Q_DECLARE_PUBLIC(PlayListManager)
    Q_DECLARE_TR_FUNCTIONS(PlayListManager)
public:
    PlayListManagerPrivate(PlayListManager *q) : q_ptr(q)
    {
        if(instance)
            qCFatal(core) << "only one instance is allowed";
        instance = q;
        qRegisterMetaType<PlayListModel::SortMode>();

        uiSettings = QmmpUiSettings::instance();
        header = new PlayListHeaderModel(q);
        timer = new QTimer(q);
        timer->setInterval(5000);
        timer->setSingleShot(true);
    }

    ~PlayListManagerPrivate()
    {
        writePlayLists();
        instance = nullptr;
    }

private:
    void writePlayLists()
    {
        qCDebug(core) << "saving playlists...";
        QString value;
        QString plFilePath = Qmmp::configDir() + u"/playlist.txt"_s;
        QSaveFile plFile(plFilePath);
        if(!plFile.open(QIODevice::WriteOnly))
        {
            qCDebug(core) << "error: %s" << plFile.errorString();
            return;
        }
        plFile.write(QStringLiteral("current_playlist=%1\n").arg(models.indexOf(currentPlayList)).toUtf8());
        for(const PlayListModel *model : std::as_const(models))
        {
            plFile.write(QStringLiteral("playlist=%1\n").arg(model->name()).toUtf8());
            if(model->isEmpty())
                continue;
            const QList<PlayListTrack *> tracks = model->tracks();
            plFile.write(QStringLiteral("current=%1\n").arg(model->currentIndex()).toLatin1());
            for(PlayListTrack *t : std::as_const(tracks))
            {
                plFile.write(QStringLiteral("file=%1\n").arg(t->path()).toUtf8());

                for(QMap<QString, Qmmp::MetaData>::const_iterator it = metaKeys.constBegin(); it != metaKeys.constEnd(); ++it)
                {
                    if(!(value = t->value(it.value())).isEmpty())
                    {
                        if(it.value() == Qmmp::COMMENT)
                        {
                            value.replace(QChar::LineFeed, QStringLiteral("\\n"));
                            value.replace(QChar::CarriageReturn, QStringLiteral("\\r"));
                        }

                        plFile.write(QStringLiteral("%1=%2\n").arg(it.key(), value).toUtf8());
                    }
                }

                for(QMap<QString, Qmmp::TrackProperty>::const_iterator it = propKeys.constBegin(); it != propKeys.constEnd(); ++it)
                {
                    if(!(value = t->value(it.value())).isEmpty())
                        plFile.write(QStringLiteral("%1=%2\n").arg(it.key(), value).toLatin1());
                }

                if(t->duration() > 0)
                    plFile.write(QStringLiteral("duration=%1\n").arg(t->duration()).toLatin1());
            }
        }
        plFile.commit();
    }

    void onListChanged(int flags)
    {
        if((flags & PlayListModel::STRUCTURE) && uiSettings->autoSavePlayList())
            timer->start();
    }

    void onCurrentTrackRemoved(const PlayListModel *sender)
    {
        Q_Q(PlayListManager);
        if(sender == currentPlayList)
            emit q->currentTrackRemoved();
    }

    void readPlayLists()
    {
        Q_Q(PlayListManager);
        Qmmp::MetaData metaKey;
        Qmmp::TrackProperty propKey;
        QString line, key, value;
        int current = 0, pl = 0;
        QList<PlayListTrack *> tracks;
        QFile file(Qmmp::configDir() + u"/playlist.txt"_s);
        if(file.open(QIODevice::ReadOnly) && file.size() > 0)
        {
            QByteArray array = file.readAll();
            file.close();
            QBuffer buffer(&array);
            buffer.open(QIODevice::ReadOnly);

            while(!buffer.atEnd())
            {
                line = QString::fromUtf8(buffer.readLine().constData()).trimmed();
                int s = line.indexOf(QLatin1Char('='));
                if (s < 0)
                    continue;

                key = line.left(s);
                value = line.right(line.size() - s - 1);

                if(key == "current_playlist"_L1)
                    pl = value.toInt();
                else if(key == "playlist"_L1)
                {
                    if(!models.isEmpty() && !tracks.isEmpty())
                    {
                        models.last()->addTracks(tracks);
                        models.last()->setCurrent(tracks.at(qBound(0, current, tracks.count() - 1)));
                    }
                    tracks.clear();
                    current = 0;
                    models << new PlayListModel(value, q);
                }
                else if (key == "current"_L1)
                {
                    current = value.toInt();
                }
                else if (key == "file"_L1)
                {
                    tracks << new PlayListTrack();
                    tracks.last()->setPath(value);
                }
                else if (tracks.isEmpty())
                    continue;
                else if (key == "duration"_L1)
                    tracks.last()->setDuration(value.toInt());
                else if (key == "length"_L1)
                    tracks.last()->setDuration(value.toInt() * 1000);
                else if((metaKey = metaKeys.value(key, Qmmp::UNKNOWN)) != Qmmp::UNKNOWN)
                {
                    if(metaKey == Qmmp::COMMENT)
                    {
                        value.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
                        value.replace(QStringLiteral("\\r"), QStringLiteral("\r"));
                    }
                    tracks.last()->setValue(metaKey, value);
                }
                else if((propKey = propKeys.value(key, Qmmp::UNKNOWN_PROPERTY)) != Qmmp::UNKNOWN_PROPERTY)
                    tracks.last()->setValue(propKey, value);
            }
            buffer.close();
        }

        if(models.isEmpty())
        {
            models << new PlayListModel(tr("Playlist"), q);
        }
        else if(!tracks.isEmpty())
        {
            models.last()->addTracks(tracks);
            models.last()->setCurrent(tracks.at(qBound(0, current, tracks.count() - 1)));
        }
        if(pl < 0 || pl >= models.count())
            pl = 0;
        selectedPlayList = models.at(pl);
        currentPlayList = models.at(pl);
        for(const PlayListModel *model : std::as_const(models))
        {
            q->connect(model, &PlayListModel::nameChanged, q, &PlayListManager::playListsChanged);
            q->connect(model, &PlayListModel::listChanged, q,  [this](int flags) { onListChanged(flags); });
            q->connect(model, &PlayListModel::currentTrackRemoved, q, [model,this] { onCurrentTrackRemoved(model); });
        }
    }

    PlayListManager *q_ptr;
    QList<PlayListModel *> models;
    PlayListModel *currentPlayList = nullptr;
    PlayListModel *selectedPlayList = nullptr;
    QTimer *timer;
    PlayListHeaderModel *header;
    QmmpUiSettings *uiSettings;
    static PlayListManager *instance;
    static const QMap<QString, Qmmp::MetaData> metaKeys;
    static const QMap<QString, Qmmp::TrackProperty> propKeys;
};

PlayListManager *PlayListManagerPrivate::instance = nullptr;

//key names
const QMap<QString, Qmmp::MetaData> PlayListManagerPrivate::metaKeys = {
    { u"title"_s, Qmmp::TITLE },
    { u"artist"_s, Qmmp::ARTIST },
    { u"albumartist"_s, Qmmp::ALBUMARTIST },
    { u"album"_s, Qmmp::ALBUM },
    { u"comment"_s, Qmmp::COMMENT },
    { u"genre"_s, Qmmp::GENRE },
    { u"composer"_s, Qmmp::COMPOSER },
    { u"year"_s, Qmmp::YEAR },
    { u"track"_s, Qmmp::TRACK },
    { u"disk"_s, Qmmp::DISCNUMBER }
};

const QMap<QString, Qmmp::TrackProperty> PlayListManagerPrivate::propKeys = {
    { u"bitrate"_s, Qmmp::BITRATE },
    { u"samplerate"_s, Qmmp::SAMPLERATE },
    { u"channels"_s, Qmmp::CHANNELS },
    { u"bits_per_sample"_s, Qmmp::BITS_PER_SAMPLE },
    { u"format_name"_s, Qmmp::FORMAT_NAME },
    { u"decoder"_s, Qmmp::DECODER },
    { u"file_size"_s, Qmmp::FILE_SIZE }
};

PlayListManager::PlayListManager(QObject *parent) :
    QObject(parent),
    d_ptr(new PlayListManagerPrivate(this))
{
    Q_D(PlayListManager);
    connect(d->timer, &QTimer::timeout, this, [d] { d->writePlayLists(); });
    //the queue reaching a track of another playlist is what makes it current
    connect(PlayListQueue::instance(), &PlayListQueue::activationRequested,
            this, [this](PlayListModel *model) {
        selectPlayList(model);
        activatePlayList(model);
    });
    d->readPlayLists(); //read playlists
}

QList<PlayListTrack *> PlayListManager::queuedTracks() const
{
    return PlayListQueue::instance()->tracks();
}

PlayListModel *PlayListManager::queuedPlayList(const PlayListTrack *track) const
{
    return PlayListQueue::instance()->owner(track);
}

bool PlayListManager::isEmptyQueue() const
{
    return PlayListQueue::instance()->isEmpty();
}

int PlayListManager::queueSize() const
{
    return PlayListQueue::instance()->count();
}

void PlayListManager::reorderQueue(const QList<PlayListTrack *> &tracks)
{
    PlayListQueue::instance()->reorder(tracks);
}

PlayListManager::~PlayListManager()
{
    delete d_ptr;
}

PlayListManager *PlayListManager::instance()
{
    return PlayListManagerPrivate::instance;
}

PlayListModel *PlayListManager::selectedPlayList() const
{
    return d_ptr->selectedPlayList;
}

PlayListModel *PlayListManager::currentPlayList() const
{
    return d_ptr->currentPlayList;
}

int PlayListManager::selectedPlayListIndex() const
{
    return indexOf(d_ptr->selectedPlayList);
}

int PlayListManager::currentPlayListIndex() const
{
    return indexOf(d_ptr->currentPlayList);
}

QList<PlayListModel *> PlayListManager::playLists() const
{
    return d_ptr->models;
}

QStringList PlayListManager::playListNames() const
{
    QStringList names;
    for(const PlayListModel *model : std::as_const(d_ptr->models))
        names << model->name();
    return names;
}

void PlayListManager::selectPlayList(PlayListModel *model)
{
    Q_D(PlayListManager);
    if(model != d->selectedPlayList && d->models.contains(model))
    {
        PlayListModel *prev = d->selectedPlayList;
        d->selectedPlayList = model;
        emit selectedPlayListChanged(model, prev);
        emit playListsChanged();
    }
}

void PlayListManager::selectPlayListIndex(int i)
{
    if(i < 0 || i > d_ptr->models.count() - 1)
        return;
    selectPlayList(playListAt(i));
}

void PlayListManager::selectPlayListName(const QString &name)
{
    int index = playListNames().indexOf(name);
    if(index >= 0)
        selectPlayList(playListAt(index));
}

void PlayListManager::selectNextPlayList()
{
    Q_D(PlayListManager);
    selectPlayListIndex(d->models.indexOf(d->selectedPlayList) + 1);
}

void PlayListManager::selectPreviousPlayList()
{
    Q_D(PlayListManager);
    selectPlayListIndex(d->models.indexOf(d->selectedPlayList) - 1);
}

void PlayListManager::activatePlayList(PlayListModel *model)
{
    Q_D(PlayListManager);
    if(model != d->currentPlayList && d->models.contains(model))
    {
        PlayListModel *prev = d->currentPlayList;
        d->currentPlayList = model;
        emit currentPlayListChanged(model, prev);
        emit playListsChanged();
    }
}

void PlayListManager::activatePlayListIndex(int index)
{
    activatePlayList(playListAt(index));
}

void PlayListManager::activateSelectedPlayList()
{
    activatePlayList(selectedPlayList());
}

PlayListModel *PlayListManager::createPlayList(const QString &name)
{
    Q_D(PlayListManager);
    PlayListModel *model = new PlayListModel(name.isEmpty() ? tr("Playlist") : name, this);
    QStringList names = playListNames();
    QString uniqueName = model->name();
    int i = 0;

    while(names.contains(uniqueName))
        uniqueName = model->name() + QStringLiteral(" (%1)").arg(++i);

    model->setName(uniqueName);

    d->models.append(model);
    connect(model, &PlayListModel::nameChanged, this, &PlayListManager::playListsChanged);
    connect(model, &PlayListModel::listChanged, this, [d](int flags) { d->onListChanged(flags); });
    connect(model, &PlayListModel::currentTrackRemoved, this, [model,d] { d->onCurrentTrackRemoved(model); });
    emit playListAdded(d->models.indexOf(model));
    selectPlayList(model);
    return model;
}

void PlayListManager::removePlayList(PlayListModel *model)
{
    Q_D(PlayListManager);
    if(d->models.count() < 2 || !d->models.contains(model))
        return;

    int i = d->models.indexOf(model);

    if(d->currentPlayList == model)
    {
        d->currentPlayList = d->models.at((i > 0) ? (i - 1) : (i + 1));
        emit currentPlayListChanged(d->currentPlayList, model);
        emit currentTrackRemoved();
    }
    if(d->selectedPlayList == model)
    {
        d->selectedPlayList = d->models.at((i > 0) ? (i - 1) : (i + 1));
        emit selectedPlayListChanged(d->selectedPlayList, model);
    }
    d->models.removeAt(i);
    //the model outlives this call, so its share of the queue is dropped here
    //rather than waiting for the container to be destroyed with it
    PlayListQueue::instance()->removeAll(model);
    model->deleteLater();
    emit playListRemoved(i);
    emit playListsChanged();
}

void PlayListManager::removePlayListIndex(int index)
{
    removePlayList(playListAt(index));
}

void PlayListManager::move(int i, int j)
{
    Q_D(PlayListManager);
    if(i < 0 || j < 0 || i == j)
        return;
    if(i < d->models.count() && j < d->models.count())
    {
        d->models.move(i,j);
        emit playListMoved(i,j);
        emit playListsChanged();
    }
}

int PlayListManager::count() const
{
    return d_ptr->models.count();
}

int PlayListManager::indexOf(PlayListModel *model) const
{
    return d_ptr->models.indexOf(model);
}

PlayListModel *PlayListManager::playListAt(int i) const
{
    Q_D(const PlayListManager);
    if(i >= 0 && i < d->models.count())
        return d->models.at(i);
    return nullptr;
}

PlayListHeaderModel *PlayListManager::headerModel()
{
    return d_ptr->header;
}

void PlayListManager::clear()
{
    d_ptr->selectedPlayList->clear();
}

void PlayListManager::clearSelection()
{
    d_ptr->selectedPlayList->clearSelection();
}

void PlayListManager::removeSelected()
{
    d_ptr->selectedPlayList->removeSelected();
}

void PlayListManager::removeUnselected()
{
    d_ptr->selectedPlayList->removeUnselected();
}

void PlayListManager::removeTrack(int i)
{
    d_ptr->selectedPlayList->removeTrack(i);
}

void PlayListManager::removeTrack(PlayListTrack *track)
{
    d_ptr->selectedPlayList->removeTrack(track);
}

void PlayListManager::invertSelection()
{
    d_ptr->selectedPlayList->invertSelection();
}

void PlayListManager::selectAll()
{
    d_ptr->selectedPlayList->selectAll();
}

void PlayListManager::showDetails()
{
    d_ptr->selectedPlayList->showDetails();
}

void PlayListManager::addTracks(const QList<PlayListTrack *> &tracks)
{
    d_ptr->selectedPlayList->addTracks(tracks);
}

void PlayListManager::addPath(const QString &path)
{
    d_ptr->selectedPlayList->addPath(path);
}

void PlayListManager::addPaths(const QStringList &paths)
{
    d_ptr->selectedPlayList->addPaths(paths);
}

void PlayListManager::randomizeList()
{
    d_ptr->selectedPlayList->randomizeList();
}

void PlayListManager::reverseList()
{
    d_ptr->selectedPlayList->reverseList();
}

void PlayListManager::sortSelection(PlayListModel::SortMode mode)
{
    d_ptr->selectedPlayList->sortSelection(mode);
}

void PlayListManager::sort(PlayListModel::SortMode mode)
{
    d_ptr->selectedPlayList->sort(mode);
}

void PlayListManager::addToQueue()
{
    d_ptr->selectedPlayList->addToQueue();
}

void PlayListManager::removeInvalidTracks()
{
    d_ptr->selectedPlayList->removeInvalidTracks();
}

void PlayListManager::removeDuplicates()
{
    d_ptr->selectedPlayList->removeDuplicates();
}

void PlayListManager::refresh()
{
    d_ptr->selectedPlayList->refresh();
}

void PlayListManager::clearQueue()
{
    d_ptr->selectedPlayList->clearQueue();
}

void PlayListManager::stopAfterSelected()
{
    d_ptr->selectedPlayList->stopAfterSelected();
}

void PlayListManager::rebuildGroups()
{
    for(PlayListModel *model : std::as_const(d_ptr->models))
        model->rebuildGroups();
}
