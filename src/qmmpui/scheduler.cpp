/***************************************************************************
 *   Copyright (C) 2026 by x-AMP developers                                *
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
#include <QCoreApplication>
#include <QProcess>
#include <QSettings>
#ifdef QMMPUI_HAS_DBUS
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#endif
#include <qmmp/qmmp.h>
#include "mediaplayer.h"
#include "playlistmanager.h"
#include "playlistmodel.h"
#include "playlisttrack.h"
#include "uihelper.h"
#include "scheduler.h"

//the countdown is checked at most once per this amount of time, so a change of
//the system clock or a resume from sleep can not overshoot the deadline for long
static constexpr int MAX_TICK_MS = 30000;

Scheduler *Scheduler::m_instance = nullptr;

Scheduler::Scheduler(QObject *parent) : QObject(parent)
{
    if(m_instance)
        qCFatal(core) << "only one instance is allowed";
    m_instance = this;

    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer); //a coarse timer may drift by 5% of the interval
    connect(&m_timer, &QTimer::timeout, this, [this] { onTimeout(); });
    readSettings();
    rearm();
}

Scheduler::~Scheduler()
{
    m_instance = nullptr;
}

Scheduler *Scheduler::instance()
{
    return m_instance;
}

bool Scheduler::isEnabled() const
{
    return m_enabled;
}

Scheduler::Trigger Scheduler::trigger() const
{
    return m_trigger;
}

QTime Scheduler::time() const
{
    return m_time;
}

int Scheduler::interval() const
{
    return m_interval;
}

Scheduler::Action Scheduler::action() const
{
    return m_action;
}

QString Scheduler::filePath() const
{
    return m_filePath;
}

QString Scheduler::playListName() const
{
    return m_playListName;
}

QDateTime Scheduler::deadline() const
{
    return m_deadline;
}

bool Scheduler::isArmedForPlayListEnd() const
{
    return m_enabled && m_trigger == PLAYLIST_END;
}

void Scheduler::setSettings(bool enabled, Trigger trigger, const QTime &time, int interval,
                            Action action, const QString &filePath, const QString &playListName)
{
    m_enabled = enabled;
    m_trigger = trigger;
    m_time = time.isValid() ? time : QTime(23, 0);
    m_interval = qMax(1, interval);
    m_action = action;
    m_filePath = filePath;
    m_playListName = playListName;
    writeSettings();
    rearm();
}

void Scheduler::setEnabled(bool enabled)
{
    if(m_enabled == enabled)
        return;

    m_enabled = enabled;
    writeSettings();
    rearm();
}

void Scheduler::readSettings()
{
    QSettings s;
    s.beginGroup(u"Scheduler"_s);
    m_enabled = s.value(u"enabled"_s, false).toBool();
    m_trigger = static_cast<Trigger>(s.value(u"trigger"_s, AT_TIME).toInt());
    m_time = s.value(u"time"_s, QTime(23, 0)).toTime();
    m_interval = qMax(1, s.value(u"interval"_s, 60).toInt());
    m_action = static_cast<Action>(s.value(u"action"_s, QUIT).toInt());
    m_filePath = s.value(u"file"_s).toString();
    m_playListName = s.value(u"playlist"_s).toString();
    s.endGroup();

    if(m_trigger < AT_TIME || m_trigger > PLAYLIST_END)
        m_trigger = AT_TIME;
    if(m_action < PLAY_FILE || m_action > SHUTDOWN)
        m_action = QUIT;
    if(!m_time.isValid())
        m_time = QTime(23, 0);
}

void Scheduler::writeSettings()
{
    QSettings s;
    s.beginGroup(u"Scheduler"_s);
    s.setValue(u"enabled"_s, m_enabled);
    s.setValue(u"trigger"_s, static_cast<int>(m_trigger));
    s.setValue(u"time"_s, m_time);
    s.setValue(u"interval"_s, m_interval);
    s.setValue(u"action"_s, static_cast<int>(m_action));
    s.setValue(u"file"_s, m_filePath);
    s.setValue(u"playlist"_s, m_playListName);
    s.endGroup();
}

void Scheduler::rearm()
{
    m_timer.stop();
    m_deadline = QDateTime();

    if(m_enabled)
    {
        switch(m_trigger)
        {
        case AT_TIME:
        {
            const QDateTime now = QDateTime::currentDateTime();
            QDateTime dt(now.date(), m_time);
            if(dt <= now)
                dt = dt.addDays(1); //the time of the day has already passed, wait for tomorrow
            m_deadline = dt;
            break;
        }
        case AFTER_INTERVAL:
            //the countdown starts over whenever the settings are applied or the player restarts
            m_deadline = QDateTime::currentDateTime().addSecs(qint64(m_interval) * 60);
            break;
        case PLAYLIST_END:
            break;
        }
    }

    if(m_deadline.isValid())
    {
        qCDebug(core) << "scheduler armed for" << m_deadline.toString(Qt::ISODate);
        scheduleTick();
    }

    emit settingsChanged();
}

void Scheduler::scheduleTick()
{
    const qint64 ms = QDateTime::currentDateTime().msecsTo(m_deadline);
    m_timer.start(static_cast<int>(qBound(qint64(100), ms, qint64(MAX_TICK_MS))));
}

void Scheduler::onTimeout()
{
    if(!m_enabled || !m_deadline.isValid())
        return;

    if(QDateTime::currentDateTime() < m_deadline)
    {
        scheduleTick();
        return;
    }
    execute();
}

void Scheduler::execute()
{
    m_timer.stop();
    m_deadline = QDateTime();
    const Action action = m_action;
    //one-shot: the scheduler disarms itself so it never fires twice
    m_enabled = false;
    writeSettings();
    emit settingsChanged();
    emit triggered(action);

    qCDebug(core) << "scheduler triggered, action:" << action;

    switch(action)
    {
    case PLAY_FILE:
        playFile();
        break;
    case PLAY_PLAYLIST:
        playPlayList();
        break;
    case QUIT:
        quitPlayer();
        break;
    case SUSPEND:
        powerRequest(true);
        break;
    case SHUTDOWN:
        powerRequest(false);
        break;
    }
}

void Scheduler::playFile()
{
    MediaPlayer *player = MediaPlayer::instance();
    if(!player || m_filePath.isEmpty())
        return;

    PlayListModel *model = player->playListManager()->currentPlayList();
    if(playPath(model, m_filePath))
        return;

    //the file is not in the playlist yet, so it is added and played once the loader is done
    const QString path = m_filePath;
    connect(model, &PlayListModel::loaderFinished, this, [this, model, path] {
        playPath(model, path);
    }, Qt::SingleShotConnection);
    model->addPath(path);
}

bool Scheduler::playPath(PlayListModel *model, const QString &path)
{
    MediaPlayer *player = MediaPlayer::instance();
    if(!player)
        return false;

    const QList<PlayListTrack *> tracks = model->tracks();
    for(PlayListTrack *track : tracks)
    {
        if(track->path() != path)
            continue;

        player->stop();
        player->playListManager()->activatePlayList(model);
        model->setCurrent(track);
        player->play();
        return true;
    }
    return false;
}

void Scheduler::playPlayList()
{
    MediaPlayer *player = MediaPlayer::instance();
    if(!player || m_playListName.isEmpty())
        return;

    PlayListManager *manager = player->playListManager();
    PlayListModel *model = nullptr;
    const QList<PlayListModel *> playLists = manager->playLists();
    for(PlayListModel *pl : playLists)
    {
        if(pl->name() == m_playListName)
        {
            model = pl;
            break;
        }
    }

    if(!model)
    {
        qCWarning(core) << "scheduler: no playlist named" << m_playListName;
        return;
    }

    auto start = [player, manager, model] {
        if(model->isEmpty())
            return;
        player->stop();
        manager->selectPlayList(model);
        manager->activatePlayList(model);
        model->setCurrent(0); //the playlist starts from its first track
        player->play();
    };

    if(model->isLoaderRunning())
        connect(model, &PlayListModel::loaderFinished, this, start, Qt::SingleShotConnection);
    else
        start();
}

void Scheduler::quitPlayer()
{
    if(UiHelper *helper = UiHelper::instance())
        helper->exit();
    else
        qApp->quit();
}

void Scheduler::powerRequest(bool suspend)
{
    //playback holds a sleep inhibitor while it runs, which would block the suspend request
    if(MediaPlayer *player = MediaPlayer::instance())
        player->stop();

#ifdef Q_OS_WIN
    if(suspend)
        QProcess::startDetached(u"rundll32.exe"_s, { u"powrprof.dll,SetSuspendState"_s, u"0,1,0"_s });
    else
        QProcess::startDetached(u"shutdown"_s, { u"/s"_s, u"/t"_s, u"0"_s });
    if(!suspend)
        quitPlayer();
    return;
#else
#ifdef QMMPUI_HAS_DBUS
    QDBusInterface interface(u"org.freedesktop.login1"_s, u"/org/freedesktop/login1"_s,
                             u"org.freedesktop.login1.Manager"_s, QDBusConnection::systemBus());
    if(interface.isValid())
    {
        const QString method = suspend ? u"Suspend"_s : u"PowerOff"_s;
        //interactive = true, so a polkit agent may ask the user for authorization
        const QDBusMessage reply = interface.call(method, true);
        if(reply.type() != QDBusMessage::ErrorMessage)
        {
            if(!suspend)
                quitPlayer();
            return;
        }
        qCWarning(core) << "scheduler: login1" << method << "failed:" << reply.errorMessage();
    }
#endif
    //no session manager answered, fall back to systemctl
    QProcess::startDetached(u"systemctl"_s, { suspend ? u"suspend"_s : u"poweroff"_s });
    if(!suspend)
        quitPlayer();
#endif
}
