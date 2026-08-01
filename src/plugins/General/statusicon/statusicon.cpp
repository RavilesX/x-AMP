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

#include <QMenu>
#include <QSettings>
#include <QDir>
#include <QTimer>
#include <QCoreApplication>
#include <QWheelEvent>
#include <QEvent>
#include <QStyle>
#include <QApplication>
#include <qmmp/soundcore.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/uihelper.h>
#include "qmmptrayicon.h"
#include "statusicon.h"

StatusIcon::StatusIcon(QObject *parent) : QObject(parent)
{
    m_tray = new QmmpTrayIcon(this);
    connect(m_tray, &QmmpTrayIcon::activated, this, &StatusIcon::trayActivated);
    //m_tray->show();
    m_core = SoundCore::instance();
    m_player = MediaPlayer::instance();
    QSettings settings;
    settings.beginGroup(u"Tray"_s);
    m_showMessage = settings.value(u"show_message"_s, false).toBool();
    m_messageDelay = settings.value(u"message_delay"_s, 2000).toInt();
    m_useStandardIcons = settings.value(u"use_standard_icons"_s, false).toBool();
    m_showToolTip = settings.value(u"show_tooltip"_s, true).toBool();
    m_splitFileName = settings.value(u"split_file_name"_s, true).toBool();
#ifdef QMMP_WS_X11
    m_toolTipTemplate = settings.value(u"tooltip_template"_s, DEFAULT_TEMPLATE).toString();
#else
    m_toolTipTemplate = u"%p%if(%p&%t, - ,)%t"_s;
#endif
    m_toolTipFormatter.setPattern(m_toolTipTemplate);
    m_messageFormatter.setPattern(u"%p%if(%p&%t, - ,)%t"_s);
    if(m_useStandardIcons)
        m_tray->setIcon(QApplication::style ()->standardIcon(QStyle::SP_MediaStop));
    else
        m_tray->setIcon(QIcon(u":/tray_stop.png"_s));
    settings.endGroup();
    //actions
    m_menu = new QMenu();
    QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
    QIcon pauseIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPause);
    QIcon stopIcon = QApplication::style()->standardIcon(QStyle::SP_MediaStop);
    QIcon nextIcon = QApplication::style()->standardIcon(QStyle::SP_MediaSkipForward);
    QIcon previousIcon = QApplication::style()->standardIcon(QStyle::SP_MediaSkipBackward);
    QIcon exitIcon = QIcon::fromTheme(u"application-exit"_s);
    m_menu->addAction(playIcon, tr("Play"), m_player, &MediaPlayer::play);
    m_menu->addAction(pauseIcon, tr("Pause"), m_player, &MediaPlayer::pause);
    m_menu->addAction(stopIcon, tr("Stop"), m_player, &MediaPlayer::stop);
    m_menu->addSeparator();
    m_menu->addAction(nextIcon, tr("Next"), m_player, &MediaPlayer::next);
    m_menu->addAction(previousIcon, tr("Previous"), m_player, &MediaPlayer::previous);
    m_menu->addSeparator();
    m_menu->addAction(exitIcon, tr("Exit"), UiHelper::instance(), &UiHelper::exit);
    m_tray->setContextMenu(m_menu);
    m_tray->show();
    connect(m_core, &SoundCore::trackInfoChanged, this, &StatusIcon::showMetaData);
    connect(m_core, &SoundCore::stateChanged, this, &StatusIcon::setState);
    setState(m_core->state()); //update state
    if (m_core->state() == Qmmp::Playing) //show test message
        QTimer::singleShot(1500, this, &StatusIcon::showMetaData);
}

StatusIcon::~StatusIcon()
{
    delete m_menu;
}

void StatusIcon::setState(Qmmp::State state)
{
    switch ((uint) state)
    {
    case Qmmp::Playing:
    {
         if(m_useStandardIcons)
            m_tray->setIcon(QApplication::style ()->standardIcon(QStyle::SP_MediaPlay));
        else
            m_tray->setIcon (QIcon(u":/tray_play.png"_s));
        break;
    }
    case Qmmp::Paused:
    {
         if(m_useStandardIcons)
            m_tray->setIcon(QApplication::style ()->standardIcon(QStyle::SP_MediaPause));
        else
            m_tray->setIcon (QIcon(u":/tray_pause.png"_s));
        break;
    }
    case Qmmp::Stopped:
    {
        if(m_useStandardIcons)
            m_tray->setIcon(QApplication::style ()->standardIcon(QStyle::SP_MediaStop));
        else
            m_tray->setIcon (QIcon(u":/tray_stop.png"_s));
        if(m_showToolTip)
            m_tray->setToolTip(tr("Stopped"));
        break;
    }
    }
}

void StatusIcon::showMetaData()
{
    TrackInfo info = m_core->trackInfo();
    if(m_splitFileName && info.value(Qmmp::TITLE).isEmpty() && !info.path().contains(u"://"_s))
    {
        QString name = QFileInfo(info.path()).completeBaseName();
        if(name.contains(QLatin1Char('-')))
        {
            info.setValue(Qmmp::TITLE, name.section(QLatin1Char('-'), 1, 1).trimmed());
            if(info.value(Qmmp::ARTIST).isEmpty())
                info.setValue(Qmmp::ARTIST, name.section(QLatin1Char('-'), 0, 0).trimmed());
        }
    }

    QString message = m_messageFormatter.format(info);
    if (message.isEmpty())
        message = info.path().section(QLatin1Char('-'), -1);

    if (m_showMessage)
        m_tray->showMessage (tr("Now Playing"), message,
                             QSystemTrayIcon::Information, m_messageDelay);

    if(m_showToolTip)
    {
        message = m_toolTipFormatter.format(info);
        if(message.isEmpty())
            message = info.path().section(QLatin1Char('-'), -1);
        m_tray->setToolTip(message);
    }
}

void StatusIcon::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
        UiHelper::instance()->toggleVisibility();
    else if (reason == QSystemTrayIcon::MiddleClick)
    {
        if (SoundCore::instance()->state() == Qmmp::Stopped)
            m_player->play();
        else
            m_player->pause();
    }
}
