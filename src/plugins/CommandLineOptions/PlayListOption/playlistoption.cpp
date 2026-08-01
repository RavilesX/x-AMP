/***************************************************************************
 *   Copyright (C) 2011-2026 by Ilya Kotov                                 *
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

#include <QtPlugin>
#include <QLocale>
#include <qmmp/soundcore.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/metadataformatter.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/commandlinemanager.h>
#include "playlistoption.h"

void PlayListOption::registerOprions()
{
    registerOption(PL_HELP,  u"--pl-help"_s, tr("Show playlist manipulation commands"));
    registerOption(PL_LIST,  u"--pl-list"_s, tr("List all available playlists"));
    registerOption(PL_DUMP,  u"--pl-dump"_s, tr("Show playlist content"), { u"id"_s });
    registerOption(PL_SELECT, u"--pl-select"_s, tr("Select playlist"), { u"id"_s });
    registerOption(PL_CREATE, u"--pl-create"_s, tr("Create playlist"), { u"name"_s });
    registerOption(PL_PLAY,  u"--pl-play"_s, tr("Play track in the specified playlist"), { u"id"_s, u"track"_s });
    registerOption(PL_CLEAR, u"--pl-clear"_s, tr("Clear playlist"), { u"id"_s });
    registerOption(PL_DELETE, u"--pl-delete"_s, tr("Remove playlist"), { u"id"_s });
    registerOption(PL_NEXT,  u"--pl-next"_s, tr("Activate next playlist"));
    registerOption(PL_PREV,  u"--pl-prev"_s, tr("Activate previous playlist"));
    registerOption(PL_REPEATE_TOGGLE, u"--pl-repeat-toggle"_s, tr("Toggle playlist repeat"));
    registerOption(PL_SHUFFLE_TOGGLE, u"--pl-shuffle-toggle"_s, tr("Toggle playlist shuffle"));
    registerOption(PL_STATE, u"--pl-state"_s, tr("Show playlist options"));

    setOptionFlags(PL_HELP, NoStart);
    setOptionFlags(PL_LIST, HiddenFromHelp);
    setOptionFlags(PL_DUMP, HiddenFromHelp);
    setOptionFlags(PL_SELECT, HiddenFromHelp);
    setOptionFlags(PL_CREATE, HiddenFromHelp);
    setOptionFlags(PL_PLAY, HiddenFromHelp);
    setOptionFlags(PL_CLEAR, HiddenFromHelp);
    setOptionFlags(PL_NEXT, HiddenFromHelp);
    setOptionFlags(PL_PREV, HiddenFromHelp);
    setOptionFlags(PL_REPEATE_TOGGLE, HiddenFromHelp);
    setOptionFlags(PL_SHUFFLE_TOGGLE, HiddenFromHelp);
    setOptionFlags(PL_STATE, HiddenFromHelp);
}

QString PlayListOption::shortName() const
{
    return QLatin1String("PlayListOption");
}

QString PlayListOption::translation() const
{
    return QLatin1String(":/playlist_plugin_");
}

QString PlayListOption::executeCommand(int id, const QStringList &args, const QString &cwd)
{
    Q_UNUSED(cwd);
    QString out;
    PlayListManager *plManager = PlayListManager::instance();
    MediaPlayer *player = MediaPlayer::instance();
    QmmpUiSettings *uiSettings = (id == PL_HELP) ? nullptr : QmmpUiSettings::instance();

    switch (id)
    {
    case PL_HELP:
    {
        static const QList<Command> commands = {
            PL_LIST, PL_DUMP, PL_SELECT, PL_CREATE, PL_PLAY, PL_CLEAR, PL_DELETE,
            PL_NEXT, PL_PREV, PL_REPEATE_TOGGLE, PL_SHUFFLE_TOGGLE, PL_STATE
        };

        for(Command c : std::as_const(commands)) {
            QString line = helpString(c);
            out += CommandLineManager::formatHelpString(line) + QChar::LineFeed;
        }

        out += QStringLiteral("-----------\n");
        out += tr("Arguments:") + QChar::LineFeed;
        out += tr("%1 - index or name of the playlist").arg(u"<id>"_s) + QChar::LineFeed;
        out += tr("%1 - index of the track").arg(u"<track>"_s) + QChar::LineFeed;
        out += tr("%1 - name of the new playlist").arg(u"<name>"_s) + QChar::LineFeed;
    }
        break;
    case PL_LIST:
    {
        QStringList names = plManager->playListNames();
        int fieldWidth = QString::number(names.count()).size();
        for(int i = 0; i <  names.count(); ++i)
        {
            if(i == plManager->currentPlayListIndex())
                out += QStringLiteral("> %1. %2\n").arg(i + 1, fieldWidth).arg(names.at(i));
            else
                out += QStringLiteral("  %1. %2\n").arg(i + 1, fieldWidth).arg(names.at(i));
        }
    }
        break;
    case PL_DUMP:
    {
        MetaDataFormatter formatter(u"%p%if(%p&%t, - ,)%t%if(%p,,%if(%t,,%f))%if(%l, - %l,)"_s);
        PlayListModel *model = findPlayList(args.isEmpty() ? QString() : args.at(0));
        if(!model)
            return tr("Invalid playlist ID");

        int fieldWidth = QString::number(model->trackCount()).size();
        for(int i = 0; i < model->trackCount(); ++i)
        {
            PlayListTrack *track = model->track(i);
            if(i == model->currentIndex())
                out += QStringLiteral("> %1. %2\n").arg(i + 1, fieldWidth).arg(formatter.format(track));
            else
                out += QStringLiteral("  %1. %2\n").arg(i + 1, fieldWidth).arg(formatter.format(track));
        }
    }
        break;
    case PL_SELECT:
    {
        PlayListModel *model = findPlayList(args.isEmpty() ? QString() : args.at(0));
        if(!model)
            return tr("Invalid playlist ID");
        plManager->selectPlayList(model);
        break;
    }
    case PL_CREATE:
    {
        if(args.isEmpty() || args.constFirst().isEmpty())
            return tr("Invalid playlist name");

        QString name = args.constFirst();

        if(plManager->playListNames().contains(name))
            return tr("Playlist with name \"%1\" already exists").arg(name);

        PlayListModel *model = plManager->createPlayList(name);
        plManager->selectPlayList(model);
        break;
    }
    case PL_PLAY:
    {
        if(args.count() > 2 || args.isEmpty())
            return tr("Invalid number of arguments");

        PlayListModel *model = findPlayList(args.size() > 1 ? args.constFirst() : QString());
        if(!model)
            return tr("Invalid playlist ID");

        int trackIndex = (args.count() == 1) ? args.constFirst().toInt() - 1 : args.at(1).toInt() - 1;
        PlayListTrack *track = model->findTrack(trackIndex);
        if(!track)
            return tr("Invalid track ID");
        player->stop();
        plManager->activatePlayList(model);
        plManager->selectPlayList(model);
        model->setCurrent(track);
        player->play();
    }
        break;
    case PL_NEXT:
    {
        bool playing = SoundCore::instance()->state() == Qmmp::Playing;
        player->stop();
        plManager->selectPlayList(plManager->currentPlayList());
        plManager->selectNextPlayList();
        plManager->activateSelectedPlayList();
        if(playing)
            player->play();
    }
        break;
    case PL_PREV:
    {
        bool playing = SoundCore::instance()->state() == Qmmp::Playing;
        player->stop();
        plManager->selectPlayList(plManager->currentPlayList());
        plManager->selectPreviousPlayList();
        plManager->activateSelectedPlayList();
        if(playing)
            player->play();
    }
        break;
    case PL_CLEAR:
    {
        PlayListModel *model = findPlayList(args.isEmpty() ? QString() : args.at(0));
        if(!model)
            return tr("Invalid playlist ID");
        model->clear();
    }
        break;
    case PL_DELETE:
    {
        if(plManager->count() < 2)
            return tr("Unable to remove last remaining playlist");

        if(args.isEmpty())
            return tr("Missing playlist ID");

        PlayListModel *model = findPlayList(args.at(0));
        if(!model)
            return tr("Invalid playlist ID");
        plManager->removePlayList(model);
    }
        break;
    case PL_REPEATE_TOGGLE:
        uiSettings->setRepeatableList(!uiSettings->isRepeatableList());
        break;
    case PL_SHUFFLE_TOGGLE:
        uiSettings->setShuffle(!uiSettings->isShuffle());
        break;
    case PL_STATE:
        out += u"SHUFFLE:             "_s + boolToText(uiSettings->isShuffle()) + QChar::LineFeed;
        out += u"REPEAT PLAYLIST:     "_s + boolToText(uiSettings->isRepeatableList()) + QChar::LineFeed;
        out += u"REPEAT TRACK:        "_s + boolToText(uiSettings->isRepeatableTrack()) + QChar::LineFeed;
        out += u"NO PLAYLIST ADVANCE: "_s + boolToText(uiSettings->isNoPlayListAdvance());
        break;
    default:
        ;
    }
    return out;
}

QString PlayListOption::boolToText(bool enabled)
{
    return enabled ? u"[+]"_s : u"[-]"_s;
}

PlayListModel *PlayListOption::findPlayList(const QString &id) const
{
    PlayListManager *plManager = PlayListManager::instance();

    if(id.isEmpty())
        return plManager->currentPlayList();

    int index = -1;
    bool ok;
    index = id.toInt(&ok) - 1;
    if(!ok)
        index = plManager->playListNames().indexOf(id);

    return plManager->playListAt(index);
}
