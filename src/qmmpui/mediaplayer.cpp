/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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
#include <QString>
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include "qmmpuisettings.h"
#include "scheduler.h"
#include "mediaplayer.h"

class MediaPlayerPrivate
{
    Q_DECLARE_PUBLIC(MediaPlayer)
public:
    MediaPlayerPrivate(MediaPlayer *q) : q_ptr(q)
    {
        if(instance)
            qCFatal(core) << "only one instance is allowed";
        instance = q;

        soundCore = new SoundCore(q);
        settings = new QmmpUiSettings(q);
        plManager = new PlayListManager(q);
        scheduler = new Scheduler(q);
        finishTimer = new QTimer(q);
        finishTimer->setSingleShot(q);
        finishTimer->setInterval(500);
    }

    ~MediaPlayerPrivate()
    {
        instance = nullptr;
    }

private:
    MediaPlayer *q_ptr;

    void playNext()
    {
        Q_Q(MediaPlayer);
        if(settings->isRepeatableTrack())
        {
            q->play();
            return;
        }
        if(isScheduledPlayListEnd())
        {
            q->stop();
            scheduler->execute();
            return;
        }
        if(settings->isNoPlayListAdvance())
        {
            q->stop();
            return;
        }
        if (!plManager->currentPlayList()->next())
        {
            if(!settings->isPlayListTransitionEnabled())
            {
                q->stop();
                return;
            }
            //next playlist
            int index = plManager->currentPlayListIndex() + 1;
            PlayListModel *nextPlayList = index < plManager->count() ? plManager->playListAt(index) : nullptr;
            PlayListTrack *nextTrack = nextPlayList ? nextPlayList->currentTrack() : nullptr;
            if(nextTrack)
            {
                plManager->selectPlayList(nextPlayList);
                plManager->activatePlayList(nextPlayList);
                q->play();
            }
            else
            {
                q->stop();
            }
            return;
        }
        q->play();
    }

    //the scheduler waits for the end of the playlist and "Repeat All" must not
    //keep it waiting forever, so the loop is cut short here
    bool isScheduledPlayListEnd() const
    {
        return scheduler->isArmedForPlayListEnd() && plManager->currentPlayList()->isFinished();
    }

    void updateNextUrl()
    {
        nextUrl.clear();
        PlayListTrack *track = nullptr;

        if(!settings->isRepeatableTrack() && isScheduledPlayListEnd())
        {
            //no gapless prefetch, otherwise playback would roll over the trigger
            qCDebug(core) << "next track state: held back by the scheduler";
            return;
        }

        if(settings->isRepeatableTrack())
            track = plManager->currentPlayList()->currentTrack();
        else if(!settings->isNoPlayListAdvance())
            track = plManager->currentPlayList()->nextTrack();

        if(!track && settings->isPlayListTransitionEnabled())
        {
            int index = plManager->currentPlayListIndex() + 1;
            PlayListModel *nextPlayList = index < plManager->count() ? plManager->playListAt(index) : nullptr;
            track = nextPlayList ? nextPlayList->currentTrack() : nullptr;
        }

        if(track)
        {
            bool ok = soundCore->play(track->path(), true);
            if(ok)
            {
                nextUrl = track->path();
                qCDebug(core) << "next track state: received";
            }
            else
                qCDebug(core) << "next track state: error";
        }
        else
            qCDebug(core) << "next track state: unknown";

    }

    void processState(Qmmp::State state)
    {
        switch ((int) state)
        {
        case Qmmp::NormalError:
            soundCore->stop();
            nextUrl.clear();
            if(m_skips <= plManager->currentPlayList()->trackCount())
            {
                m_skips++;
                playNext();
            }
            break;
        case Qmmp::FatalError:
            soundCore->stop();
            nextUrl.clear();
            break;
        case Qmmp::Playing:
            finishTimer->stop();
            m_skips = 0;
            break;
        case Qmmp::Stopped:
            finishTimer->start();
            restoreMetaData(plManager->currentPlayList()->currentTrack());
            break;
        default:
            ;
        }
    }

    void updateMetaData()
    {
        TrackInfo info = soundCore->trackInfo();
        qCDebug(core) << "===== metadata ======";
        qCDebug(core) << "ARTIST =" << info.value(Qmmp::ARTIST);
        qCDebug(core) << "TITLE =" << info.value(Qmmp::TITLE);
        qCDebug(core) << "ALBUMARTIST =" << info.value(Qmmp::ALBUMARTIST);
        qCDebug(core) << "ALBUM =" << info.value(Qmmp::ALBUM);
        qCDebug(core) << "COMMENT =" << info.value(Qmmp::COMMENT);
        qCDebug(core) << "GENRE =" << info.value(Qmmp::GENRE);
        qCDebug(core) << "YEAR =" << info.value(Qmmp::YEAR);
        qCDebug(core) << "TRACK =" << info.value(Qmmp::TRACK);
        qCDebug(core) << "DISCNUMBER =" << info.value(Qmmp::DISCNUMBER);
        qCDebug(core) << "---------------------";
        qCDebug(core) << "BITRATE =" << info.value(Qmmp::BITRATE);
        qCDebug(core) << "SAMPLERATE =" << info.value(Qmmp::SAMPLERATE);
        qCDebug(core) << "CHANNELS =" << info.value(Qmmp::CHANNELS);
        qCDebug(core) << "BITS_PER_SAMPLE =" << info.value(Qmmp::BITS_PER_SAMPLE);
        qCDebug(core) << "FORMAT_NAME =" << info.value(Qmmp::FORMAT_NAME);
        qCDebug(core) << "DECODER =" << info.value(Qmmp::DECODER);
        qCDebug(core) << "FILE_SIZE =" << info.value(Qmmp::FILE_SIZE);
        qCDebug(core) << "---------------------";
        qCDebug(core) << "REPLAYGAIN_TRACK_GAIN =" << info.value(Qmmp::REPLAYGAIN_TRACK_GAIN);
        qCDebug(core) << "REPLAYGAIN_TRACK_PEAK =" << info.value(Qmmp::REPLAYGAIN_TRACK_PEAK);
        qCDebug(core) << "REPLAYGAIN_ALBUM_GAIN =" << info.value(Qmmp::REPLAYGAIN_ALBUM_GAIN);
        qCDebug(core) << "REPLAYGAIN_ALBUM_PEAK =" << info.value(Qmmp::REPLAYGAIN_ALBUM_PEAK);
        qCDebug(core) << "---------------------";
        qCDebug(core) << "DURATION =" << info.duration();
        qCDebug(core) << "== end of metadata ==";

        PlayListModel *pl = plManager->currentPlayList();
        PlayListTrack *currentTrack = pl->currentTrack();
        if(currentTrack && currentTrack->path() == info.path())
        {
            saveMetaData(currentTrack);
            currentTrack->updateMetaData(info);
            updatePlayListMetaData(currentTrack);
        }
    }

    void onCurrentTrackRemoved()
    {
        if(settings->stopAfterRemovingOfCurrentTrack())
            soundCore->stop();
    }

    void saveMetaData(const PlayListTrack *track)
    {
        if(!track)
            return;

        savedInfo = *track;
    }

    void restoreMetaData(PlayListTrack *track)
    {
        if(!track)
            return;

        //restore initial metadata for streams
        if(savedInfo.path().contains(u"://"_s) && !savedInfo.path().contains(QLatin1Char('#')) &&
            savedInfo.path() == track->path() &&
            !savedInfo.value(Qmmp::TITLE).isEmpty())
        {
            savedInfo.clear(TrackInfo::Properties | TrackInfo::ReplayGainInfo); //restore displaying metadata only
            track->updateMetaData(savedInfo);
            updatePlayListMetaData(track);
            savedInfo.clear();
        }
    }

    void updatePlayListMetaData(PlayListTrack *track)
    {
        if(!track)
            return;

        PlayListModel *pl = plManager->currentPlayList();
        PlayListGroup *group = pl->group(track);
        //update group titles
        if(group && group->tracks().constFirst() == track)
            group->updateMetaData();

        pl->updateMetaData();
    }

    QmmpUiSettings *settings;
    PlayListManager *plManager;
    SoundCore *soundCore;
    Scheduler *scheduler;
    static MediaPlayer *instance;
    int m_skips = 0;
    QString nextUrl;
    TrackInfo savedInfo;
    QTimer *finishTimer;
};

MediaPlayer *MediaPlayerPrivate::instance = nullptr;

MediaPlayer::MediaPlayer(QObject *parent):
    QObject(parent),
    d_ptr(new MediaPlayerPrivate(this))
{
    Q_D(MediaPlayer);

    QTranslator *translator = new QTranslator(qApp);
    if(translator->load(QStringLiteral(":/libqmmpui_") + Qmmp::systemLanguageID()))
        qApp->installTranslator(translator);
    else
        delete translator;

    connect(d->finishTimer, &QTimer::timeout, this, &MediaPlayer::playbackFinished);
    connect(d->soundCore, &SoundCore::nextTrackRequest, this, [d] { d->updateNextUrl(); });
    connect(d->soundCore, &SoundCore::finished, this, [d] { d->playNext(); });
    connect(d->soundCore, &SoundCore::stateChanged, this, [d](Qmmp::State newState) { d->processState(newState); });
    connect(d->soundCore, &SoundCore::trackInfoChanged, this, [d] { d->updateMetaData(); });
    connect(d->plManager, &PlayListManager::currentTrackRemoved, this, [d] { d->onCurrentTrackRemoved(); });
}

MediaPlayer::~MediaPlayer()
{
    delete d_ptr;
}

MediaPlayer *MediaPlayer::instance()
{
    return MediaPlayerPrivate::instance;
}

PlayListManager *MediaPlayer::playListManager()
{
    return d_ptr->plManager;
}

void MediaPlayer::play()
{
    playFromPosition(-1);
}

void MediaPlayer::playFromPosition(qint64 offset)
{
    Q_D(MediaPlayer);
    d->plManager->currentPlayList()->doCurrentVisibleRequest();
    if(d->soundCore->state() == Qmmp::Paused)
    {
        d->soundCore->pause();
        return;
    }

    if(d->plManager->currentPlayList()->isEmpty())
        return;

    QString s = d->plManager->currentPlayList()->currentTrack()->path();
    if(s.isEmpty())
    {
        d->nextUrl.clear();
        return;
    }
    if(d->nextUrl == s)
    {
        d->nextUrl.clear();
        return;
    }
    d->soundCore->play(s, false, offset);
}

void MediaPlayer::stop()
{
    Q_D(MediaPlayer);
    d->soundCore->stop();
    d->nextUrl.clear();
    d->m_skips = 0;
}

void MediaPlayer::pause()
{
    d_ptr->soundCore->pause();
}

void MediaPlayer::next()
{
    Q_D(MediaPlayer);
    bool playNext = d->soundCore->state() != Qmmp::Stopped;
    stop();
    if(d->plManager->currentPlayList()->next() && playNext)
        play();
}

void MediaPlayer::previous()
{
    Q_D(MediaPlayer);
    bool playNext = d->soundCore->state() != Qmmp::Stopped;
    stop();
    if(d->plManager->currentPlayList()->previous() && playNext)
        play();
}
