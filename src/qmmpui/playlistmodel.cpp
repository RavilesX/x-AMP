/***************************************************************************
 *   Copyright(C) 2006-2026 by Ilya Kotov                                  *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                    *
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
#include <QTextStream>
#include <QSet>
#include <algorithm>
#include <qmmp/metadatamanager.h>
#include "metadatahelper_p.h"
#include "playlistparser.h"
#include "playlistformat.h"
#include "playlistcontainer_p.h"
#include "playlistqueue_p.h"
#include "groupedcontainer_p.h"
#include "normalcontainer_p.h"
#include "coverloader_p.h"
#include "playlisttask_p.h"
#include "fileloader_p.h"
#include "playstate_p.h"
#include "detailsdialog.h"
#include "qmmpuisettings.h"
#include "playlistmodel.h"

class PlayListModelPrivate
{
    Q_DECLARE_PUBLIC(PlayListModel)
public:
    PlayListModelPrivate(const QString &name, PlayListModel *model) :
        q_ptr(model),
        playListName(name)
    {
        Q_Q(PlayListModel);
        loader = new FileLoader(q);
        coverLoder = new CoverLoader(q);
        task = new PlayListTask(q);

        if(uiSettings->isGroupsEnabled())
            container = new GroupedContainer;
        else
            container = new NormalContainer;

        container->setLinesPerGroup(uiSettings->linesPerGroup());

        //x-AMP fix: the play state is created in the PlayListModel
        //constructor instead. ShufflePlayState::prepare() calls
        //q->trackCount(), which dereferences q->d_ptr -- but d_ptr is not
        //assigned until this constructor returns, so with shuffle enabled in
        //the saved settings startup crashed on an uninitialized pointer.
    }

    ~PlayListModelPrivate()
    {
        delete playState;
        delete container;
    }

    /*!
     * Returns topmost row in current selection
     */
    int topmostInSelection(int row) const
    {
        if(row == 0)
            return 0;

        for(int i = row - 1; i >= 0; i--)
        {
            if(container->track(i)->isSelected())
                continue;

            return i + 1;
        }
        return 0;
    }
    /*!
     * Returns bottommost row in current selection
     */
    int bottommostInSelection(int row) const
    {
        if(row >= container->trackCount() - 1)
            return row;

        for(int i = row + 1; i < container->trackCount(); i++)
        {
            if(container->track(i)->isSelected())
                continue;

            return i - 1;
        }
        return container->trackCount() - 1;
    }

    /*!
     * Removes items from model. If \b inverted is \b false -
     * selected items will be removed, else - unselected.
     */
    void removeSelection(bool inverted = false)
    {
        QList<PlayListItem *> tracksToRemove;

        for(PlayListTrack *t : container->tracks())
        {
            if(t->isSelected() ^ inverted)
                tracksToRemove << t;
        }

        q_ptr->removeTracks(tracksToRemove);
    }

    int removeTrackInternal(int i)
    {
        if((i < 0) || (i >= container->trackCount()))
            return 0;

        int flags = 0;
        PlayListTrack *track = container->track(i);
        if(!track)
            return flags;
        if(track->isQueued())
            flags |= PlayListModel::QUEUE;
        container->removeTrack(track);
        if(stopTrack == track)
        {
            flags |= PlayListModel::STOP_AFTER;
            stopTrack = nullptr;
        }
        if(track->isSelected())
            flags |= PlayListModel::SELECTION;

        totalDuration -= track->duration();
        totalDuration = qMax(0LL, totalDuration);

        if(currentTrack == track)
        {
            flags |= PlayListModel::CURRENT;
            if(container->isEmpty())
                currentTrack = nullptr;
            else
            {
                currentTrackIndex = i > 0 ? qMin(i - 1, container->trackCount() - 1) : 0;
                currentTrack = container->track(currentTrackIndex);
                emit q_ptr->currentTrackRemoved();
            }
        }

        if(track->isUsed())
            track->deleteLater();
        else
            delete track;

        currentTrackIndex = currentTrack ? container->indexOf(currentTrack) : -1;

        flags |= PlayListModel::STRUCTURE;
        return flags;
    }

    /*!
     * Prepares play state object
     */
    void preparePlayState()
    {
        playState->prepare();
        uniquePaths.clear();
        uniquePaths.squeeze();
    }
    /*!
     * Prepares model for shuffle playing. \b yes parameter is \b true - model iterates in shuffle mode.
     */
    void prepareForShufflePlaying(bool yes)
    {
        Q_Q(PlayListModel);
        delete playState;

        if(yes)
            playState = new ShufflePlayState(q);
        else
            playState = new NormalPlayState(q);
    }
    /*!
     * Enabled/Disabled groped mode
     * \param enabled State of the groups (\b true - enabled, \b false - disabled)
     */
    void prepareGroups(bool enabled)
    {
        PlayListContainer *newContainer = nullptr;
        if(enabled)
            newContainer = new GroupedContainer;
        else
            newContainer = new NormalContainer;
        newContainer->setLinesPerGroup(uiSettings->linesPerGroup());
        newContainer->addTracks(container->takeAllTracks());
        delete container;
        container = newContainer;
        PlayListQueue::instance()->setOwner(container, q_ptr);
        if(!container->isEmpty())
            currentTrackIndex = container->indexOf(currentTrack);
        emit q_ptr->listChanged(PlayListModel::STRUCTURE);
        startCoverLoader();
    }

    void onTaskFinished()
    {
        Q_Q(PlayListModel);
        if(task->isChanged(container)) //update unchanged container only
        {
            task->clear();
            return;
        }

        QList<PlayListTrack *> queuedTracks = container->queuedTracks();

        if(task->type() == PlayListTask::SORT || task->type() == PlayListTask::SORT_SELECTION)
        {
            container->replaceTracks(task->takeResults(&currentTrack));
            container->restoreQueue(queuedTracks);
            currentTrackIndex = container->indexOf(currentTrack);
            emit q->listChanged(PlayListModel::STRUCTURE);
        }
        else if(task->type() == PlayListTask::SORT_BY_COLUMN)
        {
            container->replaceTracks(task->takeResults(&currentTrack));
            container->restoreQueue(queuedTracks);
            currentTrackIndex = container->indexOf(currentTrack);
            emit q->listChanged(PlayListModel::STRUCTURE);
            emit q->sortingByColumnFinished(task->column(), task->isReverted());
        }
        else if(task->type() == PlayListTask::REMOVE_INVALID
                 || task->type() == PlayListTask::REMOVE_DUPLICATES
                 || task->type() == PlayListTask::REFRESH)
        {
            PlayListTrack *prev_current_track = currentTrack;
            int prev_count = container->trackCount();

            container->replaceTracks(task->takeResults(&currentTrack));

            int flags = PlayListModel::METADATA;
            if(prev_count != container->trackCount())
            {
                flags = PlayListModel::STRUCTURE;
                currentTrackIndex = currentTrack ? container->indexOf(currentTrack) : -1;
                if(prev_current_track != currentTrack)
                {
                    flags |= PlayListModel::CURRENT;
                    emit q->currentTrackRemoved();
                }

                if(stopTrack && !container->contains(stopTrack))
                {
                    stopTrack = nullptr;
                    flags |= PlayListModel::STOP_AFTER;
                }

                //remove deleted tracks from queue
                QList<PlayListTrack *>::iterator it = queuedTracks.begin();
                while(it != queuedTracks.end())
                {
                    if(!container->contains(*it))
                    {
                        flags |= PlayListModel::QUEUE;
                        it = queuedTracks.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }

                preparePlayState();
            }
            container->restoreQueue(queuedTracks);
            emit q->listChanged(flags);
        }
    }

    void updateMetaData(const QStringList &paths)
    {
        Q_Q(PlayListModel);
        if(container->isEmpty())
            return;

        QSet<QString> pathsToRemove, pathsToAdd;
        QHash<QString, TrackInfo> pathsToUpdate; //path, new metadata
        QHash<QString, QList<TrackInfo>> pathsToReplace; //path, list of tracks

        //get information
        for(const QString &path : std::as_const(paths))
        {
            if(pathsToRemove.contains(path) || pathsToUpdate.contains(path) || pathsToReplace.contains(path))
                continue;

            if(!path.contains(u"://"_s)) //local file
            {
                const QList<TrackInfo> list = MetaDataManager::instance()->createPlayList(path);
                if(list.isEmpty()) //remove unavailable files
                    pathsToRemove << path;
                else if(list.count() == 1 && list.constFirst().path() == path) //update metadata of local file
                    pathsToUpdate.insert(path, list.constFirst());
                else  //replace single file by CUE tracks
                    pathsToReplace.insert(path, list);
            }
            else if(path.contains(u"://"_s) && path.contains(QLatin1Char('#'))) //CUE track
            {
                QString filePath = TrackInfo::pathFromUrl(path);
                const QList<TrackInfo> list = MetaDataManager::instance()->createPlayList(path);
                if(list.isEmpty()) {
                    //try to receive all tracks for CUE file
                    const QList<TrackInfo> fullList = MetaDataManager::instance()->createPlayList(filePath);
                    if(fullList.isEmpty()) //invalid file
                    {
                        pathsToRemove << filePath << path;
                    }
                    else if(fullList.count() == 1 && fullList.constFirst().path() == filePath) //replace CUE tracks by single file
                    {
                        if(!pathsToAdd.contains(filePath)) //replace first CUE track
                        {
                            pathsToAdd << filePath;
                            pathsToReplace.insert(path, fullList);
                        }
                        else
                        {
                            pathsToRemove << path; //remove other CUE track
                        }
                    }
                    else
                    {
                        pathsToRemove << path; //remove unavailable CUE track
                    }
                }
                else if(list.count() == 1 && list.constFirst().path() == path) //update single CUE track
                    pathsToUpdate.insert(path, list.first());
            }
        }

        //modify playlist
        QList<PlayListTrack *> tracksToRemove, tracksToAdd;

        for(int i = 0; i < container->trackCount(); ++i)
        {
            PlayListTrack *track = container->track(i);
            if(!track || tracksToRemove.contains(track)) //skip already removed tracks (prevents infinite loop)
                continue;

            if(pathsToRemove.contains(track->path()))
                tracksToRemove << track;

            if(pathsToUpdate.contains(track->path()))
                track->updateMetaData(pathsToUpdate.value(track->path()));

            if(pathsToReplace.contains(track->path()) && !tracksToRemove.contains(track))
            {
                const QList<TrackInfo> list = pathsToReplace.value(track->path());
                QList<PlayListTrack *> tracks;
                for(const TrackInfo &info : std::as_const(list))
                    tracks << new PlayListTrack(info);

                insertTracksInternal(track, tracks);
                tracksToRemove << track; //remove previous track
            }
        }

        if(!tracksToRemove.isEmpty())
            q->removeTracks(tracksToRemove);
        if(!tracksToAdd.isEmpty())
            q->addTracks(tracksToAdd);

        q->updateMetaData();
    }

    void startCoverLoader()
    {
        if(container->groupCount() > 0 && container->linesPerGroup() > 1)
        {
            const QList<PlayListGroup *> groups = container->groups();
            QStringList paths;
            for(const PlayListGroup *g : std::as_const(groups))
            {
                if(!g->isCoverLoaded() && !g->firstTrackPath().isEmpty())
                    paths << g->firstTrackPath();
            }

            coverLoder->add(paths);
        }
    }

    void setCover(const QString &path, const QImage &img)
    {
        Q_Q(PlayListModel);
        for(PlayListGroup *g : container->groups())
        {
            if(g->firstTrackPath() == path)
                g->setCover(img);
        }
        emit q->listChanged(PlayListModel::METADATA);
    }


    void insertTracksInternal(PlayListTrack *before, const QList<PlayListTrack *> &tracks)
    {
        Q_Q(PlayListModel);
        if(uiSettings->skipExistingTracks() && q->sender() == loader)
        {
            if(uniquePaths.isEmpty())
            {
                uniquePaths.reserve(container->trackCount());
                for(const PlayListTrack *track : container->tracks())
                {
                    uniquePaths.insert(track->path());
                }
            }

            QList<PlayListTrack *> uniqueTracks;
            for(PlayListTrack *track : std::as_const(tracks))
            {
                if(!uniquePaths.contains(track->path()))
                {
                    uniquePaths.insert(track->path());
                    uniqueTracks << track;
                }
            }

            if(before)
                q->insertTracks(container->indexOf(before), uniqueTracks);
            else
                q->addTracks(uniqueTracks);
        }
        else
        {
            if(before)
                q->insertTracks(container->indexOf(before), tracks);
            else
                q->addTracks(tracks);
        }
    }

private:
    PlayListModel *q_ptr;
    PlayListTrack *currentTrack = nullptr;
    PlayListTrack *stopTrack = nullptr;
    int currentTrackIndex = -1;
    PlayState *playState = nullptr; /*!< Current playing state (Normal or Shuffle) */
    qint64 totalDuration = 0;
    FileLoader *loader;
    CoverLoader *coverLoder;
    QString playListName;
    PlayListContainer *container;
    QmmpUiSettings *uiSettings = QmmpUiSettings::instance();
    PlayListTask *task;
    QSet<QString> uniquePaths;
};

PlayListModel::PlayListModel(const QString &name, QObject *parent) :
    QObject(parent),
    d_ptr(new PlayListModelPrivate(name, this))
{
    Q_D(PlayListModel);
    //created here rather than in PlayListModelPrivate's constructor: the
    //shuffle state's prepare() goes through this->d_ptr, which is only valid
    //from this point on
    if(d->uiSettings->isShuffle())
        d->playState = new ShufflePlayState(this);
    else
        d->playState = new NormalPlayState(this);
    connect(d->uiSettings, &QmmpUiSettings::groupsChanged, this, [d](bool state) { d->prepareGroups(state); });
    connect(d->uiSettings, &QmmpUiSettings::shuffleChanged, this, [d](bool state) { d->prepareForShufflePlaying(state); });
    connect(d->loader, &FileLoader::newTracksToInsert, this, [d](PlayListTrack *before, QList<PlayListTrack *> tracks) {
        d->insertTracksInternal(before, tracks);
    }, Qt::QueuedConnection);
    connect(d->loader, &FileLoader::finished, this, [d]{ d->preparePlayState(); });
    connect(d->loader, &FileLoader::finished, this, &PlayListModel::loaderFinished);
    connect(d->loader, &FileLoader::finished, this, [d]{ d->startCoverLoader(); });
    connect(d->coverLoder, &CoverLoader::ready, this, [d](const QString &path, const QImage &img) { d->setCover(path, img); });
    connect(d->task, &PlayListTask::finished, this, [d]{ d->onTaskFinished(); });
    connect(d->task, &PlayListTask::finished, this, [d]{ d->startCoverLoader(); });
    //the queue is shared by every playlist, so a change anywhere in it moves
    //the numbers shown by this one as well
    PlayListQueue::instance()->setOwner(d->container, this);
    connect(PlayListQueue::instance(), &PlayListQueue::changed, this, [this] {
        emit listChanged(QUEUE);
    });
}

PlayListModel::~PlayListModel()
{
    blockSignals(true);
    d_ptr->loader->finish();
    d_ptr->coverLoder->finish();
    clear();
    delete d_ptr;
}

QString PlayListModel::name() const
{
    return d_ptr->playListName;
}

void PlayListModel::setName(const QString &name)
{
    Q_D(PlayListModel);
    if(d->playListName != name)
    {
        d->playListName = name;
        emit nameChanged(name);
    }
}

void PlayListModel::addTrack(PlayListTrack *track)
{
    Q_D(PlayListModel);
    d->container->addTrack(track);
    d->totalDuration += track->duration();

    int flags = 0;

    if(d->container->trackCount() == 1)
    {
        d->currentTrack = track;
        d->currentTrackIndex = d->container->indexOf(track);
        flags |= CURRENT;
    }
    else if(d->uiSettings->isGroupsEnabled())
    {
        //update current index for grouped container only
        d->currentTrackIndex = d->container->indexOf(d->currentTrack);
    }
    if(sender() != d->loader)
    {
        d->preparePlayState();
        d->startCoverLoader();
    }
    flags |= STRUCTURE;
    emit tracksAdded({ track });
    emit listChanged(flags);
}

void PlayListModel::addTracks(const QList<PlayListTrack *> &tracks)
{
    Q_D(PlayListModel);
    if(tracks.isEmpty())
        return;

    int flags = 0;

    d->container->addTracks(tracks);

    if(d->container->trackCount() == tracks.count())
    {
        d->currentTrack = tracks.constFirst();
        d->currentTrackIndex = d->container->indexOf(d->currentTrack);
        flags |= CURRENT;
    }
    else if(d->uiSettings->isGroupsEnabled())
    {
        //update current index for grouped container only
        d->currentTrackIndex = d->container->indexOf(d->currentTrack);
    }

    for(PlayListTrack *track : std::as_const(tracks))
    {
        d->totalDuration += track->duration();
    }
    emit tracksAdded(tracks);

    if(sender() != d->loader)
    {
        d->preparePlayState();
        d->startCoverLoader();
    }
    flags |= STRUCTURE;
    emit listChanged(flags);
}

void PlayListModel::addPath(const QString &path)
{
    d_ptr->loader->add(path);
}

void PlayListModel::addPaths(const QStringList &paths)
{
    d_ptr->loader->add(paths);
}

void PlayListModel::insertTrack(int index, PlayListTrack *track)
{
    Q_D(PlayListModel);
    d->container->insertTrack(index, track);
    d->totalDuration += track->duration();

    int flags = 0;

    if(d->container->trackCount() == 1)
    {
        d->currentTrack = track;
        d->currentTrackIndex = d->container->indexOf(track);
        flags |= CURRENT;
    }
    else
    {
        //update current index
        d->currentTrackIndex = d->container->indexOf(d->currentTrack);
    }
    if(sender() != d->loader)
    {
        d->preparePlayState();
        d->startCoverLoader();
    }
    emit tracksAdded({ track });
    flags |= STRUCTURE;
    emit listChanged(flags);
}

void PlayListModel::insertTracks(int index, const QList<PlayListTrack *> &tracks)
{
    Q_D(PlayListModel);
    if(tracks.isEmpty())
        return;

    int flags = 0;

    for(PlayListTrack *track : std::as_const(tracks))
    {
        index = d->container->insertTrack(index, track) + 1;
        d->totalDuration += track->duration();
        if(d->container->trackCount() == 1)
        {
            d->currentTrack = track;
            d->currentTrackIndex = d->container->indexOf(track);
            flags |= CURRENT;
        }
    }
    emit tracksAdded(tracks);

    //update current index
    d->currentTrackIndex = d->container->indexOf(d->currentTrack);
    if(sender() != d->loader)
    {
        d->preparePlayState();
        d->startCoverLoader();
    }
    flags |= STRUCTURE;
    emit listChanged(flags);
}

void PlayListModel::insertJson(int index, const QByteArray &json)
{
    insertTracks(index, PlayListParser::deserialize(json));
}

void PlayListModel::insertPath(int index, const QString &path)
{
    insertPaths(index, QStringList() << path);
}

void PlayListModel::insertPaths(int index, const QStringList &paths)
{
    Q_D(PlayListModel);
    if(index < 0 || index >= d->container->trackCount())
        addPaths(paths);
    else
    {
        PlayListTrack *before = d->container->track(index);
        d->loader->insert(before, paths);
    }
}

void PlayListModel::insertUrls(int index, const QList<QUrl> &urls)
{
    QStringList paths;
    for(const QUrl &url : std::as_const(urls))
    {
        if(url.scheme() == "file"_L1)
            paths.append(QFileInfo(url.toLocalFile()).canonicalFilePath());
        else
            paths.append(url.toString());
    }
    insertPaths(index, paths);
}

int PlayListModel::groupCount() const
{
    return d_ptr->container->groupCount();
}

int PlayListModel::trackCount() const
{
    return d_ptr->container->trackCount();
}

bool PlayListModel::isEmpty() const
{
    return d_ptr->container->isEmpty();
}

int PlayListModel::columnCount() const
{
    return MetaDataHelper::instance()->columnCount();
}

PlayListTrack *PlayListModel::currentTrack() const
{
    Q_D(const PlayListModel);
    return d->container->isEmpty() ? nullptr : d->currentTrack;
}

PlayListTrack *PlayListModel::nextTrack() const
{
    Q_D(const PlayListModel);
    if(d->container->isEmpty() || !d->playState)
        return nullptr;
    if(d->stopTrack && d->stopTrack == currentTrack())
        return nullptr;
    //the head of the queue comes first whatever playlist it belongs to
    if(PlayListTrack *queued = PlayListQueue::instance()->head())
        return queued;
    int index = d->playState->nextIndex();
    if(index < 0 || (index + 1 > d->container->trackCount()))
        return nullptr;
    return d->container->track(index);
}

bool PlayListModel::isFinished() const
{
    Q_D(const PlayListModel);
    if(d->container->isEmpty() || !d->playState)
        return true;
    //the queue spans every playlist, so anything waiting in it still has to be played
    if(!PlayListQueue::instance()->isEmpty())
        return false;
    return d->playState->isFinished();
}

int PlayListModel::indexOf(PlayListItem *item) const
{
    return d_ptr->container->indexOf(item);
}

PlayListTrack* PlayListModel::track(int index) const
{
    return d_ptr->container->track(index);
}

PlayListGroup* PlayListModel::group(int index) const
{
    return d_ptr->container->group(index);
}

PlayListGroup *PlayListModel::group(const PlayListTrack *track) const
{
    for(int i = 0; i < d_ptr->container->groupCount(); ++i)
    {
        if(d_ptr->container->group(i)->contains(track))
            return d_ptr->container->group(i);
    }

    return nullptr;
}

QList<PlayListGroup *> PlayListModel::groups() const
{
    return d_ptr->container->groups();
}

int PlayListModel::currentIndex() const
{
    return d_ptr->currentTrackIndex;
}

bool PlayListModel::setCurrent(int index)
{
    Q_D(PlayListModel);
    if(index > trackCount() - 1 || index < 0)
        return false;
    PlayListTrack *track = d->container->track(index);
    d_ptr->currentTrackIndex = index;
    d_ptr->currentTrack = track;
    emit listChanged(CURRENT);
    return true;
}

bool PlayListModel::setCurrent(PlayListTrack *track)
{
    return setCurrent(d_ptr->container->indexOf(track));
}

bool PlayListModel::next()
{
    Q_D(PlayListModel);
    if(d->stopTrack == currentTrack())
    {
        d->stopTrack = nullptr;
        emit listChanged(STOP_AFTER);
        return false;
    }
    PlayListQueue *queue = PlayListQueue::instance();
    if(!queue->isEmpty())
    {
        //the queue spans every playlist, so the track waiting at its head may
        //well belong to another one: that playlist is made the current one
        //and takes over from here
        PlayListModel *owner = queue->headOwner();
        PlayListTrack *track = queue->takeHead();
        if(owner && owner != this)
        {
            owner->setCurrent(track);
            emit queue->activationRequested(owner);
            return true;
        }
        d->currentTrack = track;
        d->currentTrackIndex = d->container->indexOf(track);
        emit listChanged(CURRENT | QUEUE);
        return true;
    }

    if(d->loader->isRunning())
        d->playState->prepare();
    return d->playState->next();
}

bool PlayListModel::previous()
{
    Q_D(PlayListModel);
    if(d->loader->isRunning())
        d->playState->prepare();
    return d->playState->previous();
}

int PlayListModel::lineCount() const
{
    return d_ptr->container->lineCount();
}

PlayListItem *PlayListModel::itemAtLine(int lineIndex) const
{
    return d_ptr->container->itemAtLine(lineIndex);
}

PlayListTrack *PlayListModel::trackAtLine(int lineIndex) const
{
    Q_D(const PlayListModel);
    int l = d->container->trackIndexAtLine(lineIndex);
    return l >= 0 ? d->container->track(l) : nullptr;
}

QList<PlayListItem *> PlayListModel::itemsAtLines(int pos, int count) const
{
    return d_ptr->container->itemsAtLines(pos, count);
}

int PlayListModel::findLine(const PlayListItem *item) const
{
    Q_D(const PlayListModel);
    if(!item)
        return -1;

    for(int i = 0; i < d->container->lineCount(); ++i)
    {
        if(d->container->itemAtLine(i) == item)
            return i;
    }
    return -1;
}

int PlayListModel::findLine(int trackIndex) const
{
    return findLine(d_ptr->container->track(trackIndex));
}

int PlayListModel::subIndexOfLine(int lineIndex) const
{
    return d_ptr->container->subIndexOfLine(lineIndex);
}

int PlayListModel::trackIndexAtLine(int lineIndex) const
{
    return d_ptr->container->trackIndexAtLine(lineIndex);
}

bool PlayListModel::alternateColor(int lineIndex) const
{
    return d_ptr->container->alternateColor(lineIndex);
}

int PlayListModel::linesPerGroup() const
{
    return d_ptr->container->linesPerGroup();
}

void PlayListModel::clear()
{
    Q_D(PlayListModel);
    d->loader->finish();
    d->coverLoder->finish();
    d->currentTrackIndex = -1;
    if(d->currentTrack)
    {
        d->currentTrack = nullptr;
        emit currentTrackRemoved();
    }
    d->stopTrack = nullptr;
    d->container->clear();
    d->totalDuration = 0;
    d->playState->resetState();
    emit listChanged(STRUCTURE | QUEUE | STOP_AFTER | CURRENT | SELECTION);
}

void PlayListModel::clearSelection()
{
    d_ptr->container->clearSelection();
    emit listChanged(SELECTION);
}

bool PlayListModel::contains(const QString &url)
{
    Q_D(PlayListModel);
    for(int i = 0; i < d->container->trackCount(); ++i)
    {
        PlayListTrack *t = d->container->track(i);
        if(t->path() == url)
            return true;
    }
    return false;
}

PlayListTrack *PlayListModel::findTrack(int trackIndex) const
{
    return d_ptr->container->track(trackIndex);
}

QList<PlayListItem *> PlayListModel::findTracks(const QString &str) const
{
    Q_D(const PlayListModel);
    QList<PlayListItem *> items;
    PlayListItem *item = nullptr;
    if(str.isEmpty())
        return items;

    for(int i = 0; i < d->container->trackCount(); ++i)
    {
        item = d->container->track(i);

        if(!item->formattedTitles().filter(str, Qt::CaseInsensitive).isEmpty())
            items.append(item);
    }
    return items;
}

void PlayListModel::setSelected(PlayListItem *item, bool selected)
{
    if(item)
    {
        item->setSelected(selected);
        emit listChanged(SELECTION);
    }
}

void PlayListModel::setSelected(const QList<PlayListTrack *> &tracks, bool selected)
{
    for(PlayListTrack *t : std::as_const(tracks))
        t->setSelected(selected);
    emit listChanged(SELECTION);
}

void PlayListModel::setSelected(const QList<PlayListItem *> &items, bool selected)
{
    for(PlayListItem *i : std::as_const(items))
        i->setSelected(selected);
    emit listChanged(SELECTION);
}

void PlayListModel::setSelectedLines(int firstLine, int lastLine, bool selected)
{
    Q_D(PlayListModel);
    if(firstLine > lastLine)
    {
        setSelectedLines(lastLine, firstLine, selected);
        return;
    }

    for(int index = firstLine; index <= lastLine; ++index)
    {
        PlayListItem *item = d->container->itemAtLine(index);
        if(item)
            item->setSelected(selected);
    }

    emit listChanged(SELECTION);
}

void PlayListModel::removeSelected()
{
    d_ptr->removeSelection(false);
}

void PlayListModel::removeUnselected()
{
    d_ptr->removeSelection(true);
}

void PlayListModel::removeTrack(int i)
{
    int flags = d_ptr->removeTrackInternal(i);
    if(flags)
    {
        emit listChanged(flags);
        d_ptr->preparePlayState();
    }
}

void PlayListModel::removeTrack(PlayListTrack *track)
{
    Q_D(PlayListModel);
    if(d->container->contains(track))
        removeTrack(d->container->indexOf(track));
}

void PlayListModel::removeTracks(const QList<PlayListItem *> &items)
{
    Q_D(PlayListModel);
    int i = 0;
    int select_after_delete = -1;
    int flags = 0;

    while (!d->container->isEmpty() && i < d->container->trackCount())
    {
        PlayListItem *item = d->container->track(i);
        if(!item->isGroup() && items.contains(item))
        {
            flags |= d->removeTrackInternal(i);

            if(d->container->isEmpty())
                continue;

            select_after_delete = i;
        }
        else
        {
            i++;
        }
    }

    select_after_delete = qMin(select_after_delete, d->container->trackCount() - 1);

    if(select_after_delete >= 0)
    {
        d->container->track(select_after_delete)->setSelected(true);
        flags |= SELECTION;
    }

    d->preparePlayState();

    if(flags)
        emit listChanged(flags);
}

void PlayListModel::removeTracks(const QList<PlayListTrack *> &tracks)
{
    QList<PlayListItem *> items;
    for(PlayListTrack *track : tracks)
        items << static_cast<PlayListItem *>(track);

    removeTracks(items);
}

void PlayListModel::invertSelection()
{
    Q_D(PlayListModel);
    for(int i = 0; i < d->container->trackCount(); ++i)
    {
        PlayListTrack *track = d->container->track(i);
        track->setSelected(!track->isSelected());
    }
    for(int i = 0; i < d->container->groupCount(); ++i)
    {
        PlayListGroup *group = d->container->group(i);
        group->setSelected(!group->isSelected());
    }
    emit listChanged(SELECTION);
}

void PlayListModel::selectAll()
{
    Q_D(PlayListModel);
    for(int i = 0; i < d->container->trackCount(); ++i)
        d->container->track(i)->setSelected(true);
    for(int i = 0; i < d->container->groupCount(); ++i)
        d->container->group(i)->setSelected(true);
    emit listChanged(SELECTION);
}

void PlayListModel::showDetails(QWidget *parent)
{
    Q_D(PlayListModel);
    QList<PlayListTrack *> selected_tracks = selectedTracks();

    if(!selected_tracks.isEmpty())
    {
        DetailsDialog *dialog = new DetailsDialog(selected_tracks, parent);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(dialog, &DetailsDialog::metaDataChanged, this, [d](const QStringList &paths) { d->updateMetaData(paths); });
        dialog->show();
    }
}

void PlayListModel::showDetailsForCurrent(QWidget *parent)
{
    Q_D(PlayListModel);
    if(d->currentTrack)
    {
        DetailsDialog *dialog = new DetailsDialog(QList<PlayListTrack *>() << d->currentTrack, parent);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(dialog, &DetailsDialog::metaDataChanged, this, [d](const QStringList &paths) { d->updateMetaData(paths); });
        dialog->show();
    }
}

int PlayListModel::firstSelectedLine() const
{
    Q_D(const PlayListModel);
    for(int i = 0; i < d->container->lineCount(); i++)
    {
        if(d->container->itemAtLine(i)->isSelected())
            return i;
    }
    return -1;
}

int PlayListModel::lastSelectedLine() const
{
    Q_D(const PlayListModel);
    for(int i = d->container->lineCount() - 1; i >= 0; i--)
    {
        if(d->container->itemAtLine(i)->isSelected())
            return i;
    }
    return -1;
}

qint64 PlayListModel::totalDuration() const
{
    return d_ptr->totalDuration;
}

void PlayListModel::moveTracks(int from, int to)
{
    Q_D(PlayListModel);
    // Get rid of useless work
    if(from == to || from < 0 || to < 0)
        return;

    QList<int> selected_indexes = selectedTrackIndexes();
    QList<PlayListGroup *> groups = d->container->groups();

    if(selected_indexes.isEmpty())
        return;

    if(std::any_of(groups.cbegin(), groups.cend(), [](PlayListGroup *g){ return g->isSelected(); }))
        return;

    if(from == -1 || d->bottommostInSelection(from) == -1 || d->topmostInSelection(from) == -1)
        return;

    if(d->container->move(selected_indexes, from, to))
    {
        d->currentTrackIndex = d->container->indexOf(d->currentTrack);
        emit listChanged(STRUCTURE);
    }
}

SimpleSelection PlayListModel::getSelection(int trackIndex) const
{
    Q_D(const PlayListModel);
    SimpleSelection sel = { .top = d->topmostInSelection(trackIndex), .bottom = d->bottommostInSelection(trackIndex) };
    return sel;
}

QList<int> PlayListModel::selectedLines() const
{
    Q_D(const PlayListModel);
    QList<int> lines;
    for(int i = 0; i < d->container->lineCount(); i++)
    {
        if(d->container->itemAtLine(i)->isSelected())
            lines.append(i);
    }
    return lines;
}

void PlayListModel::setSelectedLine(int line, bool selected)
{
    PlayListItem *item = d_ptr->container->itemAtLine(line);
    if(item)
    {
        item->setSelected(selected);
        emit listChanged(SELECTION);
    }
}

QList<int> PlayListModel::selectedTrackIndexes() const
{
    Q_D(const PlayListModel);
    QList<int> selected_rows;
    for(int i = 0; i < d->container->trackCount(); i++)
    {
        if(d->container->track(i)->isSelected())
        {
            selected_rows.append(i);
        }
    }
    return selected_rows;
}

QList<PlayListTrack *> PlayListModel::selectedTracks() const
{
    Q_D(const PlayListModel);
    QList<PlayListTrack *> selected_tracks;
    for(PlayListTrack *track : d->container->tracks())
    {
        if(track->isSelected())
            selected_tracks << track;
    }
    return selected_tracks;
}

QList<PlayListTrack *> PlayListModel::tracks() const
{
    return d_ptr->container->tracks();
}

void PlayListModel::addToQueue()
{
    const QList<PlayListTrack*> selected_tracks = selectedTracks();
    blockSignals(true);
    for(PlayListTrack *track : std::as_const(selected_tracks))
        setQueued(track);
    blockSignals(false);
    emit listChanged(QUEUE);
}

void PlayListModel::setQueued(PlayListTrack *t)
{
    Q_D(const PlayListModel);
    if(t->isQueued())
        d->container->removeFromQueue(t);
    else
        d->container->enqueue(t);
    emit listChanged(QUEUE);
}

QList<PlayListTrack *> PlayListModel::queuedTracks() const
{
    return d_ptr->container->queuedTracks();
}

bool PlayListModel::isEmptyQueue() const
{
    return d_ptr->container->queuedTracks().isEmpty();
}

int PlayListModel::queueSize() const
{
    return d_ptr->container->queuedTracks().count();
}

bool PlayListModel::isStopAfter(const PlayListItem *track) const
{
    return d_ptr->stopTrack == track;
}

void PlayListModel::randomizeList()
{
    Q_D(PlayListModel);
    if(d->container->isEmpty())
        return;
    d->container->randomizeList();
    d->currentTrackIndex = d->container->indexOf(d->currentTrack);
    emit listChanged(STRUCTURE);
}

void PlayListModel::reverseList()
{
    Q_D(PlayListModel);
    if(d->container->isEmpty())
        return;
    d->container->reverseList();
    d->currentTrackIndex = d->container->indexOf(d->currentTrack);
    emit listChanged(STRUCTURE);
}

void PlayListModel::sortSelection(PlayListModel::SortMode mode)
{
    Q_D(PlayListModel);
    if(d->container->isEmpty())
        return;

    d->task->sortSelection(d->container->tracks(), mode);
}

void PlayListModel::sort(PlayListModel::SortMode mode)
{
    Q_D(PlayListModel);
    if(d->container->isEmpty())
        return;

    d->task->sort(d->container->tracks(), mode);
}

void PlayListModel::sortByColumn(int column)
{
    Q_D(PlayListModel);
    if(d->container->isEmpty())
        return;

    if(column < 0 || column >= columnCount())
        return;

    d->task->sortByColumn(d->container->tracks(), column);
}

void PlayListModel::updateMetaData()
{
    emit listChanged(METADATA);
}

void PlayListModel::doCurrentVisibleRequest()
{
    Q_D(PlayListModel);
    if(!d->container->isEmpty() && d->currentTrackIndex >= 0)
        emit scrollToRequest(d->currentTrackIndex);
}

void PlayListModel::scrollTo(int trackIndex)
{
    Q_D(PlayListModel);
    if(trackIndex >= 0 && trackIndex < d->container->trackCount())
        emit scrollToRequest(trackIndex);
}

void PlayListModel::loadPlaylist(const QString &f_name)
{
    d_ptr->loader->add(f_name);
}

void PlayListModel::loadPlaylist(const QString &fmt, const QByteArray &data)
{
    d_ptr->loader->addPlayList(fmt, data);
}

void PlayListModel::savePlaylist(const QString &f_name)
{
    PlayListParser::savePlayList(d_ptr->container->tracks(), f_name);
}

bool PlayListModel::isLoaderRunning() const
{
    return d_ptr->loader->isRunning();
}

void PlayListModel::removeInvalidTracks()
{
    Q_D(PlayListModel);
    d->task->removeInvalidTracks(d->container->tracks(), d->currentTrack);
}

void PlayListModel::removeDuplicates()
{
    Q_D(PlayListModel);
    d->task->removeDuplicates(d->container->tracks(), d->currentTrack);
}

void PlayListModel::refresh()
{
    Q_D(PlayListModel);
    d->task->refresh(d->container->tracks(), d->currentTrack);
}

void PlayListModel::clearQueue()
{
    Q_D(PlayListModel);
    //one queue for all the playlists, so clearing it clears the whole thing
    //rather than this playlist's share of it
    PlayListQueue::instance()->clear();
    d->stopTrack = nullptr;
    emit listChanged(QUEUE);
}

void PlayListModel::stopAfterSelected()
{
    Q_D(PlayListModel);
    QList<PlayListTrack *> selected_tracks = selectedTracks();

    if(!isEmptyQueue())
    {
        d->stopTrack = d->stopTrack != d->container->queuedTracks().constLast() ? d->container->queuedTracks().constLast() : nullptr;
        emit listChanged(STOP_AFTER);
    }
    else if(selected_tracks.count() == 1)
    {
        d->stopTrack = d->stopTrack != selected_tracks.constFirst() ? selected_tracks.constFirst() : nullptr;
        emit listChanged(STOP_AFTER);
    }
    else if(selected_tracks.count() > 1)
    {
        blockSignals(true);
        addToQueue();
        blockSignals(false);
        d->stopTrack = d->container->queuedTracks().constLast();
        emit listChanged(STOP_AFTER | QUEUE);
    }
}

void PlayListModel::rebuildGroups()
{
    Q_D(PlayListModel);
    if(d->uiSettings->isGroupsEnabled())
        d->prepareGroups(true);
}
