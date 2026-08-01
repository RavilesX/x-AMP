/***************************************************************************
 *   Copyright (C) 2009-2012 by Artur Guzik                                *
 *   a.guzik88@gmail.com                                                   *
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

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QTimer>
#include <QImage>
#include <QApplication>
#include <QVariant>
#include <QStandardPaths>
#include <qmmpui/metadataformatter.h>
#include <qmmp/soundcore.h>
#include <qmmp/metadatamanager.h>

#include "kdenotify.h"

KdeNotify::KdeNotify(QObject *parent) : QObject(parent)
{

    m_notifier = new QDBusInterface(u"org.freedesktop.Notifications"_s,
                                    u"/org/freedesktop/Notifications"_s,
                                    u"org.freedesktop.Notifications"_s,
                                    QDBusConnection::sessionBus(), this);
    if(m_notifier->lastError().type() != QDBusError::NoError)
    {
        qCWarning(plugin) << "Unable to create interface:" << m_notifier->lastError().message();
        return;
    }

    qCDebug(plugin) << "DBus interface created successfully.";
    QDir dir(Qmmp::cacheDir());
    if(!dir.exists(u"kdenotifycache"_s))
        dir.mkdir(u"kdenotifycache"_s);
    dir.cd(u"kdenotifycache"_s);
    m_coverPath = dir.absolutePath() + u"/cover.jpg"_s;
    m_imagesDir = Qmmp::dataPath() + u"/images"_s;

    QSettings settings;
    settings.beginGroup(u"Kde_Notifier"_s);
    m_notifyDuration = settings.value(u"notify_duration"_s, 5000).toInt();
    m_showCovers = settings.value(u"show_covers"_s, true).toBool();
    m_template = settings.value(u"template"_s, DEFAULT_TEMPLATE).toString();
    m_template.remove(QChar::LineFeed);
    m_updateNotify = settings.value(u"update_notify"_s, true).toBool();
    m_currentNotifyId = 0;

    if(m_updateNotify)
    {
        connect(SoundCore::instance(), &SoundCore::trackInfoChanged, this, &KdeNotify::showMetaData);
        connect(m_notifier, SIGNAL(NotificationClosed(uint,uint)), this, SLOT(notificationClosed(uint,uint)));
    }
    else
    {
        QTimer *timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(NOTIFY_DELAY); //after that notification will be showed.
        connect(timer, &QTimer::timeout, this, &KdeNotify::showMetaData);
        connect(SoundCore::instance(), &SoundCore::trackInfoChanged, this, [=] { timer->start(); } );
    }

    if(settings.value(u"volume_notification"_s, false).toBool())
    {
        connect(SoundCore::instance(), &SoundCore::volumeChanged, this, &KdeNotify::onVolumeChanged);
        connect(SoundCore::instance(), &SoundCore::mutedChanged, this, &KdeNotify::onMutedChanged);
    }
    settings.endGroup();
}

KdeNotify::~KdeNotify()
{
    QDir dir(QDir::home());
    dir.remove(m_coverPath);
}

QString KdeNotify::totalTimeString()
{
    int time = SoundCore::instance()->duration() / 1000;

    if(time >= 3600)
    {
        return QStringLiteral("%1:%2:%3").arg(time / 3600, 2, 10, QLatin1Char('0')).arg(time % 3600/60, 2, 10, QLatin1Char('0'))
                .arg(time%60,2,10,QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(time / 60, 2, 10, QLatin1Char('0')).arg(time%60, 2, 10, QLatin1Char('0'));
}

QList<QVariant> KdeNotify::prepareNotification()
{
    SoundCore *core = SoundCore::instance();
    TrackInfo info = core->trackInfo();
    if(info.isEmpty()) //prevent show empty notification
    {
        return QList<QVariant>();
    }
    QList<QVariant> args;
    args.append(u"Qmmp"_s); //app-name
    args.append(m_currentNotifyId); //replaces-id;
    args.append(m_imagesDir + u"/app-icon.png"_s);  //app-icon(path to icon on disk)
    args.append(tr("Qmmp now playing:")); //summary (notification title)

    MetaDataFormatter f(m_template);
    QString body = f.format(info);
    body.replace(u"<br>"_s, u"\n"_s);

    QString coverPath;
    if(m_showCovers)
    {
        QImage cover = MetaDataManager::instance()->getCover(info.path());
        if(!cover.isNull())
        {
            coverPath = m_coverPath;
            cover.scaled(90, 90, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).save(coverPath);
        }
    }
    if(coverPath.isEmpty())
        coverPath = m_imagesDir + u"/empty_cover.png"_s;

    args.append(body); //body
    args.append(QStringList()); //actions
    QVariantMap hints;
    hints.insert(u"image-path"_s, coverPath);
    args.append(hints); //hints

    args.append(m_notifyDuration); //timeout

    return args;
}

void KdeNotify::showMetaData()
{
    QList<QVariant> n = prepareNotification();
    if(!n.isEmpty())
    {
        QDBusReply<uint> reply = m_notifier->callWithArgumentList(QDBus::Block, u"Notify"_s, n);
        if(reply.isValid() && m_updateNotify)
        {
            m_currentNotifyId = reply.value();
        }
    }
}

void KdeNotify::notificationClosed(uint id, uint reason)
{
    Q_UNUSED(reason);
    if(m_currentNotifyId == id)
        m_currentNotifyId = 0;
}

void KdeNotify::onVolumeChanged(int percent)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                      QStringLiteral("/org/kde/osdService"),
                                                      QStringLiteral("org.kde.osdService"),
                                                      QStringLiteral("mediaPlayerVolumeChanged"));
    msg.setArguments({ percent, u"Qmmp"_s, u"qmmp-simple"_s });
    QDBusConnection::sessionBus().asyncCall(msg);
}

void KdeNotify::onMutedChanged(bool muted)
{
    onVolumeChanged(muted ? 0 : SoundCore::instance()->volume());
}
