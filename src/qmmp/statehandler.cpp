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

#include <QStringList>
#include <QApplication>
#include <QMutexLocker>
#include <QMetaType>
#include "soundcore.h"
#include "qmmpevents_p.h"
#include "statehandler.h"

#define TICK_INTERVAL 250
#define PREFINISH_TIME 7000

class StateHandlerPrivate
{
public:
    ~StateHandlerPrivate()
    {
        instance = nullptr;
    }

    qint64 elapsed = -1;
    qint64 duration = 0;
    bool sendAboutToFinish = true;
    int bitrate = 0;
    TrackInfo info;
    QHash <QString, QString> streamInfo;
    Qmmp::State m_state = Qmmp::Stopped;
    AudioParameters audioParameters;
    mutable QRecursiveMutex mutex;
    static StateHandler *instance;
};

StateHandler* StateHandlerPrivate::instance = nullptr;

StateHandler::StateHandler(QObject *parent) :
    QObject(parent),
    d_ptr(new StateHandlerPrivate)
{
    if(StateHandlerPrivate::instance)
        qCFatal(core) << "only one instance is allowed";
    qRegisterMetaType<AudioParameters>("AudioParameters");
    StateHandlerPrivate::instance = this;
}

StateHandler::~StateHandler()
{
    delete d_ptr;
}

void StateHandler::dispatch(qint64 elapsed, int bitrate)
{
    Q_D(StateHandler);
    d->mutex.lock();
    if (qAbs(d->elapsed - elapsed) > TICK_INTERVAL)
    {
        d->elapsed = elapsed;
        emit elapsedChanged(elapsed);
        if (d->bitrate != bitrate)
        {
            d->bitrate = bitrate;
            emit bitrateChanged(bitrate);
        }
        if((SoundCore::instance()->duration() > PREFINISH_TIME)
                 && (d->duration - d->elapsed < PREFINISH_TIME)
                 && d->sendAboutToFinish)
        {
            d->sendAboutToFinish = false;
            if(d->duration - d->elapsed > PREFINISH_TIME / 2)
                qApp->postEvent(parent(), new QEvent(EVENT_NEXT_TRACK_REQUEST));
        }
    }
    d->mutex.unlock();
}

void StateHandler::dispatch(const AudioParameters &p)
{
    Q_D(StateHandler);
    d->mutex.lock();
    if(d->audioParameters != p)
    {
        d->audioParameters = p;
        emit audioParametersChanged(p);
    }
    d->mutex.unlock();
}

void StateHandler::dispatch(qint64 length)
{
    Q_D(StateHandler);
    d->mutex.lock();
    d->duration = length;
    d->mutex.unlock();
}

bool StateHandler::dispatch(const TrackInfo &info)
{
    Q_D(StateHandler);
    QMutexLocker locker(&d->mutex);
    if(info.isEmpty())
    {
        qCWarning(core, "empty metadata");
        return false;
    }
    if(d->m_state != Qmmp::Playing && d->m_state != Qmmp::Paused)
    {
        qCWarning(core, "metadata is ignored");
        return false;
    }

    if(d->info.isEmpty() || d->info.path() == info.path())
    {
        TrackInfo tmp = d->info;
        tmp.setPath(info.path());
        if(info.parts() & TrackInfo::MetaData)
            tmp.setValues(info.metaData());
        if(info.parts() & TrackInfo::Properties)
            tmp.setValues(info.properties());
        if(info.parts() & TrackInfo::ReplayGainInfo)
            tmp.setValues(info.replayGainInfo());
        if(info.duration() > 0)
            tmp.setDuration(info.duration());

        if(d->info != tmp)
        {
            d->info = tmp;
            qApp->postEvent(parent(), new TrackInfoEvent(d->info));
            return true;
        }
    }
    return false;
}

void StateHandler::dispatch(const QHash<QString, QString> &info)
{
    Q_D(StateHandler);
    d->mutex.lock();
    QHash<QString, QString> tmp = info;
    const auto values = tmp.values();
    for(const QString &value : values) //remove empty keys
    {
        if(value.isEmpty())
            tmp.remove(tmp.key(value));
    }
    if(d->streamInfo != tmp)
    {
        d->streamInfo = tmp;
        qApp->postEvent(parent(), new StreamInfoChangedEvent(d->streamInfo));
    }
    d->mutex.unlock();
}

void StateHandler::dispatch(Qmmp::State state)
{
    Q_D(StateHandler);
    d->mutex.lock();
    //clear
    static const QList<Qmmp::State> clearStates = { Qmmp::Stopped, Qmmp::NormalError, Qmmp::FatalError };
    if (clearStates.contains(state))
    {
        d->elapsed = -1;
        d->bitrate = 0;
        d->info.clear();
        d->streamInfo.clear();
        d->sendAboutToFinish = true;
        d->audioParameters = AudioParameters(44100, ChannelMap(2), Qmmp::PCM_UNKNOWN);
    }
    if (d->m_state != state)
    {
        static const QStringList states = {
            u"Playing"_s, u"Paused"_s, u"Stopped"_s, u"Buffering"_s, u"NormalError"_s, u"FatalError"_s
        };
        qCDebug(core) << "Current state:" << states.at(state) <<  "; previous state:" << states.at(d->m_state);
        Qmmp::State prevState = state;
        d->m_state = state;
        qApp->postEvent(parent(), new StateChangedEvent(d->m_state, prevState));
    }
    d->mutex.unlock();
}

void StateHandler::dispatchBuffer(int percent)
{
    Q_D(StateHandler);
    if(d->m_state == Qmmp::Buffering)
        emit bufferingProgress(percent);
}

qint64 StateHandler::elapsed() const
{
    Q_D(const StateHandler);
    QMutexLocker locker(&d->mutex);
    return d->elapsed;
}

qint64 StateHandler::duration() const
{
    Q_D(const StateHandler);
    QMutexLocker locker(&d->mutex);
    return d->duration;
}

int StateHandler::bitrate() const
{
    Q_D(const StateHandler);
    QMutexLocker locker(&d->mutex);
    return d->bitrate;
}

AudioParameters StateHandler::audioParameters() const
{
    Q_D(const StateHandler);
    QMutexLocker locker(&d->mutex);
    return d->audioParameters;
}

Qmmp::State StateHandler::state() const
{
    Q_D(const StateHandler);
    return d->m_state;
}

void StateHandler::sendNextTrackRequest()
{
    Q_D(StateHandler);
    d->mutex.lock();
    if(d->sendAboutToFinish)
    {
        d->sendAboutToFinish = false;
        qApp->postEvent(parent(), new QEvent(EVENT_NEXT_TRACK_REQUEST));
    }
    d->mutex.unlock();
}

void StateHandler::sendFinished()
{
    qApp->postEvent(parent(), new QEvent(EVENT_FINISHED));
}

StateHandler *StateHandler::instance()
{
    return StateHandlerPrivate::instance;
}
