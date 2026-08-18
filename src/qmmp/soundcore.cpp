/***************************************************************************
 *   Copyright (C) 2006-2026 by Ilya Kotov                                 *
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

#include <QIODevice>
#include <QFile>
#include <QApplication>
#include <QSettings>
#include <QDir>
#include "qmmpevents_p.h"
#include "qmmpaudioengine_p.h"
#include "statehandler.h"
#include "inputsource.h"
#include "volumehandler.h"
#include "enginefactory.h"
#include "metadatamanager.h"
#include "qmmpsettings.h"
#include "soundcore.h"

class SoundCorePrivate
{
    Q_DECLARE_PUBLIC(SoundCore)
public:
    SoundCorePrivate(SoundCore *core) : q_ptr(core) {}

private:
    enum NextEngineState
    {
        NO_ENGINE = 0,
        SAME_ENGINE,
        ANOTHER_ENGINE,
        INVALID_SOURCE
    };

    void startNextSource()
    {
        Q_Q(SoundCore);
        if(sources.isEmpty())
            return;

        InputSource *s = sources.dequeue();
        path = s->path();

        if(s->ioDevice() && !s->ioDevice()->isOpen() && !s->ioDevice()->open(QIODevice::ReadOnly))
        {
            qCWarning(core, "input error: %s", qPrintable(s->ioDevice()->errorString()));
            path.clear();
            s->deleteLater();
            nextState = SoundCorePrivate::INVALID_SOURCE;
            if(handler->state() == Qmmp::Stopped || handler->state() == Qmmp::Buffering)
                handler->dispatch(Qmmp::NormalError);
            return;
        }

        if(!engine)
        {
            if((engine = AbstractEngine::create(s, q)))
            {
                engine->play();
                nextState = NO_ENGINE;
                return;
            }

            s->deleteLater();
            handler->dispatch(Qmmp::NormalError);
            return;
        }

        if(AbstractEngine::isEnabled(engine) && engine->enqueue(s))
        {
            if(q->state() == Qmmp::Stopped || q->state() == Qmmp::Buffering)
            {
                engine->play();
                nextState = NO_ENGINE;
            }
            else
            {
                nextState = SAME_ENGINE;
            }
        }
        else
        {
            sources.prepend(s); //try next engine
            nextState = ANOTHER_ENGINE;
            if(q->state() == Qmmp::Stopped || q->state() == Qmmp::Buffering)
            {
                startNextEngine();
            }
        }
    }

    void startNextEngine()
    {
        Q_Q(SoundCore);
        switch(nextState)
        {
        case NO_ENGINE:
        case SAME_ENGINE:
        {
            if(sources.isEmpty())
                nextState = NO_ENGINE;
            else if(!sources.constFirst()->isReady() && q->state() == Qmmp::Stopped)
                handler->dispatch(Qmmp::Buffering);
            break;
        }
        case ANOTHER_ENGINE:
        {
            nextState = NO_ENGINE;
            if(engine)
            {
                engine->deleteLater();
                engine = nullptr;
            }
            if(!sources.isEmpty())
            {
                handler->dispatch(Qmmp::Buffering);
                startNextSource();
            }
            break;
        }
        case INVALID_SOURCE:
            handler->dispatch(Qmmp::NormalError);
        }
    }

    SoundCore *q_ptr;
    QHash <QString, QString> streamInfo;
    TrackInfo info;
    QString path;
    StateHandler *handler;
    VolumeHandler *volumeControl;
    AbstractEngine *engine = nullptr;
    QQueue<InputSource *> sources;
    NextEngineState nextState = NO_ENGINE;
    static SoundCore *instance;
};

SoundCore *SoundCorePrivate::instance = nullptr;

SoundCore::SoundCore(QObject *parent) :
    QObject(parent),
    d_ptr(new SoundCorePrivate(this))
{
    Q_D(SoundCore);
    if(SoundCorePrivate::instance)
        qCFatal(core) << "only one instance is allowed";
    qRegisterMetaType<Qmmp::State>("Qmmp::State");
    SoundCorePrivate::instance = this;
    d->handler = new StateHandler(this);
    d->volumeControl = new VolumeHandler(this);
    connect(d->handler, &StateHandler::elapsedChanged, this, &SoundCore::elapsedChanged);
    connect(d->handler, &StateHandler::bitrateChanged, this, &SoundCore::bitrateChanged);
    connect(d->handler, &StateHandler::audioParametersChanged, this, &SoundCore::audioParametersChanged);
    connect(d->handler, &StateHandler::bufferingProgress, this, &SoundCore::bufferingProgress);
    connect(QmmpSettings::instance(), &QmmpSettings::eqSettingsChanged, this, &SoundCore::eqSettingsChanged);
    connect(QmmpSettings::instance(), &QmmpSettings::audioSettingsChanged, d->volumeControl, &VolumeHandler::reload);
    connect(d->volumeControl, &VolumeHandler::volumeChanged, this, &SoundCore::volumeChanged);
    connect(d->volumeControl, &VolumeHandler::balanceChanged, this, &SoundCore::balanceChanged);
    connect(d->volumeControl, &VolumeHandler::mutedChanged, this, &SoundCore::mutedChanged);
}

SoundCore::~SoundCore()
{
    stop();
    delete d_ptr;
    SoundCorePrivate::instance = nullptr;
}

bool SoundCore::play(const QString &source, bool queue, qint64 offset)
{
    Q_D(SoundCore);
    if(!queue)
        stop();

    MetaDataManager::instance(); //create metadata manager

    InputSource *s = InputSource::create(source, this);
    s->setOffset(offset);
    d->sources.enqueue(s);

    connect(s, &InputSource::ready, this, [d] { d->startNextSource(); });
    connect(s, &InputSource::error, this, [d] { d->startNextSource(); });

    if(!s->initialize())
    {
        d->sources.removeAll(s);
        s->deleteLater();
        if(d->handler->state() == Qmmp::Stopped || d->handler->state() == Qmmp::Buffering)
            d->handler->dispatch(Qmmp::NormalError);
        return false;
    }
    if(d->handler->state() == Qmmp::Stopped)
        d->handler->dispatch(Qmmp::Buffering);
    return true;
}

void SoundCore::stop()
{
    Q_D(SoundCore);
    qApp->sendPostedEvents(this, 0);
    d->path.clear();
    qDeleteAll(d->sources);
    d->sources.clear();
    d->nextState = SoundCorePrivate::NO_ENGINE;
    if(d->engine)
    {
        d->engine->stop();
        qApp->sendPostedEvents(this, 0);
        //m_engine->deleteLater();
        //m_engine = 0;
    }
    d->volumeControl->reload();
    if(state() == Qmmp::NormalError || state() == Qmmp::FatalError || state() == Qmmp::Buffering)
        StateHandler::instance()->dispatch(Qmmp::Stopped); //clear error and buffering state
}

void SoundCore::pause()
{
    Q_D(SoundCore);
    if(d->engine)
        d->engine->pause();
}

void SoundCore::seek(qint64 pos)
{
    Q_D(SoundCore);
    if(d->engine)
        d->engine->seek(pos);
}

void SoundCore::seekRelative(qint64 offset)
{
    qint64 d = duration();
    if(d > 0)
        seek(qBound(0, elapsed() + offset, d));
}

bool SoundCore::nextTrackAccepted() const
{
    return d_ptr->nextState == SoundCorePrivate::SAME_ENGINE;
}

AudioParameters SoundCore::nextAudioParameters() const
{
    if(QmmpAudioEngine *engine = QmmpAudioEngine::instance())
        return engine->queuedAudioParameters();
    return AudioParameters();
}

const QString SoundCore::path() const
{
    return d_ptr->path;
}

qint64 SoundCore::duration() const
{
    return d_ptr->handler->duration();
}

EqSettings SoundCore::eqSettings() const
{
    return QmmpSettings::instance()->eqSettings();
}

void SoundCore::setEqSettings(const EqSettings &settings)
{
    QmmpSettings::instance()->setEqSettings(settings);
}

void SoundCore::setVolumePerChannel(int L, int R)
{
    setMuted(false);
    d_ptr->volumeControl->setVolume(L, R);
}

void SoundCore::setMuted(bool mute)
{
    d_ptr->volumeControl->setMuted(mute);
}

void SoundCore::changeVolume(int delta)
{
    setMuted(false);
    d_ptr->volumeControl->changeVolume(delta);
}

void SoundCore::setVolume(int volume)
{
    setMuted(false);
    d_ptr->volumeControl->setVolume(volume);
}

void SoundCore::volumeUp()
{
    changeVolume(QmmpSettings::instance()->volumeStep());
}

void SoundCore::volumeDown()
{
    changeVolume(-QmmpSettings::instance()->volumeStep());
}

void SoundCore::setBalance(int balance)
{
    setMuted(false);
    d_ptr->volumeControl->setBalance(balance);
}

int SoundCore::leftVolume() const
{
    return d_ptr->volumeControl->left();
}

int SoundCore::rightVolume() const
{
    return d_ptr->volumeControl->right();
}

int SoundCore::volume() const
{
    return d_ptr->volumeControl->volume();
}

int SoundCore::balance() const
{
    return d_ptr->volumeControl->balance();
}

bool SoundCore::isMuted() const
{
    return d_ptr->volumeControl->isMuted();
}

qint64 SoundCore::elapsed() const
{
    return d_ptr->handler->elapsed();
}

int SoundCore::bitrate() const
{
    return d_ptr->handler->bitrate();
}

AudioParameters SoundCore::audioParameters() const
{
    return d_ptr->handler->audioParameters();
}

Qmmp::State SoundCore::state() const
{
    return d_ptr->handler->state();
}

QMap<Qmmp::MetaData, QString> SoundCore::metaData() const
{
    return d_ptr->info.metaData();
}

QString SoundCore::metaData(Qmmp::MetaData key) const
{
    return d_ptr->info.value(key);
}

QHash<QString, QString> SoundCore::streamInfo() const
{
    return d_ptr->streamInfo;
}

TrackInfo SoundCore::trackInfo() const
{
    return d_ptr->info;
}

SoundCore *SoundCore::instance()
{
    return SoundCorePrivate::instance;
}

bool SoundCore::event(QEvent *e)
{
    Q_D(SoundCore);
    if(e->type() == EVENT_STATE_CHANGED)
    {
        Qmmp::State st = (static_cast<StateChangedEvent *>(e))->currentState();
        emit stateChanged(st);
        if(st == Qmmp::Stopped)
        {
            d->streamInfo.clear();
            d->startNextEngine();
        }
    }
    else if(e->type() == EVENT_STREAM_INFO_CHANGED)
    {
        d->streamInfo = (static_cast<StreamInfoChangedEvent *>(e))->streamInfo();
        emit streamInfoChanged();
    }
    else if(e->type() == EVENT_TRACK_INFO_CHANGED)
    {
        d->info = (static_cast<TrackInfoEvent *>(e))->trackInfo();
        emit trackInfoChanged();
    }
    else if(e->type() == EVENT_NEXT_TRACK_REQUEST)
        emit nextTrackRequest();
    else if(e->type() == EVENT_FINISHED)
        emit finished();

    return QObject::event(e);
}
