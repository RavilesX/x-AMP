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
#include <QFileInfo>
#include <qmmp/soundcore.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/playlistdownloader.h>
#include <qmmpui/playlistparser.h>
#include <qmmp/metadatamanager.h>
#include "builtincommandlineoption.h"

// BuiltinCommandLineOption methods implementation
BuiltinCommandLineOption::BuiltinCommandLineOption(QObject *parent) : QObject(parent)
{
#ifdef Q_OS_WIN
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(500);
    connect(m_timer, &QTimer::timeout, this,  &BuiltinCommandLineOption::addPendingPaths);
#endif
}

void BuiltinCommandLineOption::registerOprions()
{
    registerOption(ENQUEUE, { u"-e"_s, u"--enqueue"_s }, tr("Don't clear the playlist"));
    registerOption(PLAY, { u"-p"_s, u"--play"_s }, tr("Start playing current song"));
    registerOption(PAUSE, { u"-u"_s, u"--pause"_s }, tr("Pause current song"));
    registerOption(PLAY_PAUSE, { u"-t"_s, u"--play-pause"_s }, tr("Pause if playing, play otherwise"));
    registerOption(STOP, { u"-s"_s, u"--stop"_s }, tr("Stop current song"));
    registerOption(JUMP_TO_TRACK, { u"-j"_s, u"--jump-to-track"_s }, tr("Display Jump to Track dialog"));
    registerOption(QUIT, { u"-q"_s, u"--quit"_s }, tr("Quit application"));
    registerOption(VOLUME, u"--volume"_s, tr("Set playback volume (example: qmmp --volume 20)"), { u"0..100"_s });
    registerOption(VOLUME_STATUS, u"--volume-status"_s, tr("Print volume level"));
    registerOption(TOGGLE_MUTE, u"--toggle-mute"_s, tr("Mute/Restore volume"));
    registerOption(MUTE_STATUS, u"--mute-status"_s, tr("Print mute status"));
    registerOption(NEXT, u"--next"_s, tr("Skip forward in playlist"));
    registerOption(PREVIOUS, u"--previous"_s, tr("Skip backwards in playlist"));
    registerOption(TOGGLE_VISIBILITY, u"--toggle-visibility"_s, tr("Show/hide application"));
    registerOption(SHOW_MW, u"--show-mw"_s, tr("Show main window"));
    registerOption(ADD_FILE, u"--add-file"_s, tr("Display Add File dialog"));
    registerOption(ADD_DIRECTORY, u"--add-dir"_s, tr("Display Add Directory dialog"));
}

QString BuiltinCommandLineOption::shortName() const
{
    return u"BuiltinOptions"_s;
}

QString BuiltinCommandLineOption::translation() const
{
    return QString();
}

QString BuiltinCommandLineOption::executeCommand(int id, const QStringList &args, const QString &cwd)
{
    SoundCore *core = SoundCore::instance();
    MediaPlayer *player = MediaPlayer::instance();
    PlayListManager *pl_manager = PlayListManager::instance();
    QmmpUiSettings *settings = QmmpUiSettings::instance();
    QString out;
    if(!core || !player)
        return out;

    switch(id) {
    case OPEN:
    case ENQUEUE:
    {
        if(args.isEmpty())
            return out;
        QStringList full_path_list, remote_pls_list;
        for(const QString &s : std::as_const(args))
        {
            QString path = s;
#ifdef Q_OS_WIN
            path.replace(QLatin1Char('\\'), QLatin1Char('/'));
#endif
            if(QFileInfo(s).isAbsolute()) //absolute path
                full_path_list << path;
            else if(path.contains(u"://"_s)) //url
            {
                if(PlayListParser::findByUrl(path)) //remote playlist
                    remote_pls_list << path;
                else
                    full_path_list << path; //url
            }
            else //relative path
                full_path_list << cwd + QLatin1Char('/') + path;
        }
        //default playlist
        if(settings->useDefaultPlayList())
        {
            if(!pl_manager->playListNames().contains(settings->defaultPlayListName()))
                pl_manager->createPlayList(settings->defaultPlayListName());
            pl_manager->selectPlayListName(settings->defaultPlayListName());
        }
        pl_manager->activatePlayList(pl_manager->selectedPlayList());
        PlayListModel *model = pl_manager->selectedPlayList();

        if(id == OPEN)
        {
            model->clear(); //clear playlist if option is empty
            m_pending_path_list << full_path_list;
#ifdef Q_OS_WIN
            //windows starts instance for each selected file,
            //so we should wait paths from all started qmmp instances
            m_timer->start();
#else
            addPendingPaths();
#endif
        }
        else
            model->addPaths(full_path_list);
        if(!remote_pls_list.isEmpty())
        {
            PlayListDownloader *downloader = new PlayListDownloader(this);
            connect(downloader, &PlayListDownloader::finished, downloader, &PlayListDownloader::deleteLater);
            downloader->start(remote_pls_list.constFirst(), model);
        }
        break;
    }
    case PLAY:
        player->play();
        break;
    case STOP:
        player->stop();
        break;
    case PAUSE:
        if(core->state() == Qmmp::Playing)
            player->pause();
        break;
    case NEXT:
        player->next();
        if (core->state() == Qmmp::Stopped)
            player->play();
        break;
    case PREVIOUS:
        player->previous();
        if (core->state() == Qmmp::Stopped)
            player->play();
        break;
    case PLAY_PAUSE:
        if (core->state() == Qmmp::Playing)
            player->pause();
        else
            player->play();
        break;
    case JUMP_TO_TRACK:
        UiHelper::instance()->jumpToTrack();
        break;
    case QUIT:
        qApp->closeAllWindows();
        qApp->quit();
        break;
    case TOGGLE_VISIBILITY:
        UiHelper::instance()->toggleVisibility();
        break;
    case SHOW_MW:
        UiHelper::instance()->showMainWindow();
        break;
    case ADD_FILE:
        UiHelper::instance()->addFiles();
        break;
    case ADD_DIRECTORY:
        UiHelper::instance()->addDirectory();
        break;
    case VOLUME:
        if(!args.isEmpty())
        {
            bool ok = false;
            int volume = args.at(0).toInt(&ok);
            if (ok)
                core->setVolume(volume);
        }
        break;
    case VOLUME_STATUS:
        out += QStringLiteral("%1\n").arg(core->volume());
        break;
    case TOGGLE_MUTE:
        core->setMuted(!core->isMuted());
        break;
    case MUTE_STATUS:
        out += QStringLiteral("%1\n").arg(core->isMuted());
        break;
    }
    return out;
}

QHash<QString, QStringList> BuiltinCommandLineOption::splitArgs(const QStringList &args) const
{
    QHash <QString, QStringList> commands;
    QString lastCmd;
    for(const QString &arg : std::as_const(args))
    {
        QString cmd = arg.trimmed();
        if(cmd.startsWith(QLatin1Char('-')) || cmd.startsWith(u"--"_s))
        {
            commands.insert(cmd, QStringList());
            lastCmd = cmd;
        }
        else if(!commands.isEmpty() && !lastCmd.isEmpty())
            commands[lastCmd] << arg;
    }
    return commands;
}

void BuiltinCommandLineOption::addPendingPaths()
{
    if(m_pending_path_list.isEmpty())
        return;

    SoundCore *core = SoundCore::instance();

    if(core->state() != Qmmp::Stopped)
    {
        MediaPlayer::instance()->stop();
        qApp->processEvents(); //receive stop signal
    }

    UiHelper::instance()->replaceAndPlay(m_pending_path_list);
    m_pending_path_list.clear();
}
