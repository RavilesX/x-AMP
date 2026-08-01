/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
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

#include <QSettings>
#include <QApplication>
#include <QProcess>
#include <qmmp/soundcore.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistitem.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/metadataformatter.h>
#include "trackchange.h"

TrackChange::TrackChange(QObject *parent) : QObject(parent)
{
    m_core = SoundCore::instance();
    m_plManager = PlayListManager::instance();
    connect(m_core, &SoundCore::stateChanged, this, &TrackChange::onStateChanged);
    connect(m_core, &SoundCore::trackInfoChanged, this, &TrackChange::onTrackInfoChanged);
    connect(m_core, &SoundCore::finished, this, &TrackChange::onFinised);
    QSettings settings;
    settings.beginGroup(u"TrackChange"_s);
    m_newTrackCommand = settings.value(u"new_track_command"_s).toString();
    m_endOfTrackCommand = settings.value(u"end_of_track_command"_s).toString();
    m_endOfPlCommand = settings.value(u"end_of_pl_command"_s).toString();
    m_titleChangeCommand = settings.value(u"title_change_command"_s).toString();
    m_appStartupCommand = settings.value(u"application_startup_command"_s).toString();
    m_appExitCommand = settings.value(u"application_exit_command"_s).toString();
    settings.endGroup();

    connect(qApp, &QCoreApplication::aboutToQuit, this, &TrackChange::onAppExit);
    onAppStartup();
}

TrackChange::~TrackChange()
{}

void TrackChange::onStateChanged(Qmmp::State state)
{
    switch (state)
    {
    case Qmmp::Playing:
        break;
    default:
        m_prevInfo.clear();
    }
}

void TrackChange::onTrackInfoChanged()
{
    TrackInfo info = m_core->trackInfo();
    if(m_prevInfo.metaData() != info.metaData())
    {
        if(m_prevInfo.path() == info.path())
        {
            if(!m_titleChangeCommand.isEmpty())
            {
                qCDebug(plugin, "starting title change command..");
                executeCommand(info, m_titleChangeCommand);
            }
        }
        else
        {
            if(!m_newTrackCommand.isEmpty())
            {
                qCDebug(plugin, "starting new track command..");
                executeCommand(info, m_newTrackCommand);
            }
        }
    }
    m_prevInfo = info;
}

void TrackChange::onFinised()
{
    if(!m_endOfTrackCommand.isEmpty())
    {
        qCDebug(plugin, "starting end of track command..");
        executeCommand(m_prevInfo, m_endOfTrackCommand);
    }
    if(!m_endOfPlCommand.isEmpty() && !m_plManager->currentPlayList()->nextTrack())
    {
        qCDebug(plugin, "tarting end of playlist command..");
        executeCommand(m_prevInfo, m_endOfPlCommand);
    }
}

void TrackChange::onAppStartup()
{
    if(QApplication::allWindows().count() == 1 && !m_appStartupCommand.isEmpty()) //detect startup
    {
#ifdef Q_OS_WIN
        QProcess::startDetached(QStringLiteral("cmd.exe /C %1").arg(m_appStartupCommand));
#else
        QStringList args = { u"-c"_s, m_appStartupCommand };
        QProcess::startDetached(u"sh"_s, args);
#endif
    }
}

void TrackChange::onAppExit()
{
    if(!m_appExitCommand.isEmpty())
    {
#ifdef Q_OS_WIN
        QProcess::startDetached(QStringLiteral("cmd.exe /C %1").arg(m_appExitCommand));
#else
        QStringList args = { u"-c"_s, m_appExitCommand };
        QProcess::startDetached(u"sh"_s, args);
#endif
    }
}

bool TrackChange::executeCommand(const TrackInfo &info, const QString &format)
{
    TrackInfo tmp = info;
    QMap<Qmmp::MetaData, QString> metaData = tmp.metaData();
    QMap<Qmmp::MetaData, QString>::iterator it = metaData.begin();
    while(it != metaData.end())
    {
#ifdef Q_OS_WIN
        it.value() = it.value().replace(u"'"_s, u"^'"_s);
#else
        it.value() = it.value().replace(u"'"_s, u"'\\''"_s);
#endif
        ++it;
    }
    tmp.setValues(metaData);
    MetaDataFormatter formatter(format);
    QString command = formatter.format(tmp);
#ifdef Q_OS_WIN
    bool ok = QProcess::startDetached(QStringLiteral("cmd.exe /C %1").arg(command));
#else
    QStringList args = { u"-c"_s, command };
    bool ok = QProcess::startDetached(u"sh"_s, args);
#endif
    if(!ok)
        qCWarning(plugin, "unable to start command '%s'", qPrintable(command));
    return ok;
}
