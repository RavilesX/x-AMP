/***************************************************************************
 *   Copyright (C) 2014-2026 by Ilya Kotov                                 *
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

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusConnectionInterface>
#include <QVariantList>
#include <QDateTime>
#include <QCoreApplication>
#include <qmmpui/mediaplayer.h>
#include <qmmp/soundcore.h>
#include "mediakeys.h"

MediaKeys::MediaKeys(QObject *parent) : QObject(parent)
{
    if(!QDBusConnection::sessionBus().interface()->isServiceRegistered(u"org.gnome.SettingsDaemon"_s))
    {
        qCWarning(plugin, "gnome settings daemon is not running");
        return;
    }

    m_interface = new QDBusInterface(u"org.gnome.SettingsDaemon"_s,
                                     u"/org/gnome/SettingsDaemon/MediaKeys"_s,
                                     u"org.gnome.SettingsDaemon.MediaKeys"_s,
                                     QDBusConnection::sessionBus(), this);

    QDBusPendingReply<> reply = grabMediaPlayerKeys(QCoreApplication::applicationName(), QDateTime::currentSecsSinceEpoch());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &MediaKeys::onRegisterFinished);
}

MediaKeys::~MediaKeys()
{
    if(m_isRegistered && m_interface)
    {
        QDBusPendingReply<> reply = releaseMediaPlayerKeys(QCoreApplication::applicationName());
        reply.waitForFinished();
        qCWarning(plugin, "unregistered");
    }
}

QDBusPendingReply<> MediaKeys::grabMediaPlayerKeys(const QString &application, uint time)
{
    QVariantList argumentList = { QVariant::fromValue(application), QVariant::fromValue(time) };
    return m_interface->asyncCallWithArgumentList(u"GrabMediaPlayerKeys"_s, argumentList);
}

QDBusPendingReply<> MediaKeys::releaseMediaPlayerKeys(const QString &application)
{
    QVariantList argumentList = { QVariant::fromValue(application) };
    return m_interface->asyncCallWithArgumentList(u"ReleaseMediaPlayerKeys"_s, argumentList);
}

void MediaKeys::onRegisterFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusMessage reply = watcher->reply();
    watcher->deleteLater();

    if(reply.type() == QDBusMessage::ErrorMessage)
    {
        qCWarning(plugin, "unable to grab media keys: [%s] - %s",
                 qPrintable(reply.errorName()), qPrintable(reply.errorMessage()));
        return;
    }
    m_interface->connection().connect(u"org.gnome.SettingsDaemon"_s,
                                      u"/org/gnome/SettingsDaemon/MediaKeys"_s,
                                      u"org.gnome.SettingsDaemon.MediaKeys"_s,
                                      u"MediaPlayerKeyPressed"_s, this,
                                      SLOT(onKeyPressed(QString,QString)));
    m_isRegistered = true;
    qCDebug(plugin, "registered");

}

void MediaKeys::onKeyPressed(const QString &in0, const QString &in1)
{
    if(in0 != QCoreApplication::applicationName())
        return;
    MediaPlayer *player = MediaPlayer::instance();
    SoundCore *core = SoundCore::instance();
    qCDebug(plugin, "[%s] pressed", qPrintable(in1));
    if(in1 == "Play"_L1)
    {
        if (core->state() == Qmmp::Stopped)
            player->play();
        else if (core->state() != Qmmp::FatalError)
            player->pause();
    }
    else if(in1 == "Pause"_L1)
        player->pause();
    else if(in1 == "Stop"_L1)
        player->stop();
    else if(in1 == "Previous"_L1)
        player->previous();
    else if(in1 == "Next"_L1)
        player->next();
    else
        qCWarning(plugin, "unknown media key pressed");
}
