/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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

#include <QAction>
#include <QSettings>
#include <QApplication>
#include <QProgressDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <qmmp/soundcore.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistitem.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/metadataformatter.h>
#include "fileops.h"

#define COPY_BLOCK_SIZE 102400

FileOps::FileOps(QObject *parent) : QObject(parent)
{
    //separators
    QAction *separator1 = new QAction(this);
    separator1->setSeparator(true);
    QAction *separator2 = new QAction(this);
    separator2->setSeparator(true);
    //load settings
    QSettings settings;
    settings.beginGroup("FileOps"_L1);
    if(!settings.value("name_0"_L1).isNull())
        UiHelper::instance()->addAction(separator1, UiHelper::PLAYLIST_MENU);
    else
        return;

    int i = 0;
    while(!settings.value(QStringLiteral("name_%1").arg(i)).isNull())
    {
        QString name = settings.value(QStringLiteral("name_%1").arg(i)).toString();
        QVariantMap data = {
            { u"action"_s,  settings.value(QStringLiteral("action_%1").arg(i), FileOps::COPY).toInt() },
            { u"pattern"_s, settings.value(QStringLiteral("pattern_%1").arg(i)).toString() },
            { u"destination"_s, settings.value(QStringLiteral("destination_%1").arg(i)).toString() },
            { u"command"_s, settings.value(QStringLiteral("command_%1").arg(i)).toString() },

        };

        if(settings.value(QStringLiteral("enabled_%1").arg(i), true).toBool())
        {
            QAction *action = new QAction(name, this);
            action->setData(data);
            action->setShortcut(QKeySequence(settings.value(QStringLiteral("hotkey_%1").arg(i)).toString(), QKeySequence::PortableText));
            connect(action, &QAction::triggered, this, &FileOps::execAction);
            UiHelper::instance()->addAction(action, UiHelper::PLAYLIST_MENU);
        }

        ++i;
    }
    settings.endGroup();
    UiHelper::instance()->addAction(separator2, UiHelper::PLAYLIST_MENU);
}

FileOps::~FileOps()
{}

void FileOps::execAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    QVariantMap data = action->data().toMap();

    int type = data[u"action"_s].toInt();
    QString pattern = data[u"pattern"_s].toString();
    QString destination = data[u"destination"_s].toString();
    QString command = data[u"command"_s].toString();

    MetaDataFormatter formatter(type == EXECUTE ? command : pattern);

    PlayListModel *model = MediaPlayer::instance()->playListManager()->selectedPlayList();
    const QList<PlayListTrack*> tracks = model->selectedTracks();

    switch (type)
    {
    case COPY:
    {
        qCDebug(plugin, "copy");
        if(!QDir(destination).exists ())
        {
            QMessageBox::critical (qApp->activeWindow (), tr("Error"),
                                   tr("Destination directory doesn't exist"));
            break;
        }
        copy(tracks, destination, &formatter);
        break;
    }
    case RENAME:
    {
        qCDebug(plugin, "rename");
        rename(tracks, &formatter, model);
        break;
    }
    case REMOVE:
    {
        qCDebug(plugin, "remove");
        if(QMessageBox::question (qApp->activeWindow (), tr("Remove Files"),
                                   tr("Are you sure you want to remove %n file(s) from disk?",
                                      "",tracks.size()),
                                   QMessageBox::Yes | QMessageBox::No) !=  QMessageBox::Yes)
            break;

        if(PlayListManager::instance()->selectedPlayList() != model)
            break;

        for(PlayListTrack *track : std::as_const(tracks))
        {
            if(PlayListManager::instance()->selectedPlayList() != model)
                break;

            if(PlayListManager::instance()->currentPlayList()->currentTrack() == track)
                MediaPlayer::instance()->stop();

            if(isValid(track) && QFile::exists(track->path()) && QFile::remove(track->path()))
                model->removeTrack(track);
        }
        break;
    }
    case MOVE:
    {
        qCDebug(plugin, "move");
        if(!QDir(destination).exists ())
        {
            QMessageBox::critical (qApp->activeWindow (), tr("Error"),
                                   tr("Destination directory doesn't exist"));
            break;
        }
        if(QMessageBox::question (qApp->activeWindow (), tr("Move Files"),
                                   tr("Are you sure you want to move %n file(s)?",
                                      "",tracks.size()),
                                   QMessageBox::Yes | QMessageBox::No) !=  QMessageBox::Yes)
        {
            break;
        }
        move(tracks, destination, &formatter, model);
        break;
    }
    case EXECUTE:
    {
        qCDebug(plugin, "execute");
        execute(tracks, &formatter, model);
        break;
    }
    }
}

void FileOps::copy(const QList<PlayListTrack *> &tracks, const QString &dest, const MetaDataFormatter *formatter)
{
    QProgressDialog progress(qApp->activeWindow ());
    progress.setWindowModality(Qt::WindowModal);
    progress.setWindowTitle(tr("Copying"));
    progress.setCancelButtonText(tr("Stop"));
    progress.show();
    progress.setAutoClose(false);
    int i  = 0;
    for(PlayListTrack *track : std::as_const(tracks))
    {
        if(!isValid(track) || !QFile::exists(track->path()))
            continue;

        QString fileName = formatter->format(track); //generate file name

        QString ext = QStringLiteral(".") + track->path().section(QLatin1Char('.'), -1).toLower();
        if(!ext.isEmpty() && !fileName.endsWith(ext, Qt::CaseInsensitive))
            fileName += ext; //append extension

        //create destination path
        QString path = dest + QLatin1Char('/') + fileName;
        QDir dir = QFileInfo(path).dir();
        if(!dir.exists())
        {
            if(!dir.mkpath(dir.absolutePath()))
            {
                qCWarning(plugin, "unable to create directory");
                continue;
            }
        }
        if(track->path() == path)
            continue;

        //copy file
        QFile in(track->path());
        QFile out(path);
        if(!in.open(QIODevice::ReadOnly))
        {
            qCWarning(plugin) << in.errorString();
            continue;
        }
        if(!out.open(QIODevice::WriteOnly))
        {
            qCWarning(plugin) << out.errorString();
            continue;
        }

        progress.setMaximum(int(in.size() / COPY_BLOCK_SIZE));
        progress.setValue(0);
        progress.setLabelText (QString(tr("Copying file %1/%2")).arg(++i).arg(tracks.size()));
        progress.update();

        while (!in.atEnd ())
        {
            out.write(in.read(COPY_BLOCK_SIZE));
            progress.setValue(int(out.size() / COPY_BLOCK_SIZE));
            qApp->processEvents();
        }
        if(progress.wasCanceled ())
            break;
    }
    progress.close();
}

void FileOps::rename(const QList<PlayListTrack *> &tracks, const MetaDataFormatter *formatter, PlayListModel *model)
{
    for(PlayListTrack *track : std::as_const(tracks))
    {
        if(!isValid(track) || !QFile::exists(track->path())) //is it file?
            continue;

        if(PlayListManager::instance()->selectedPlayList() != model)
            break;

        if(PlayListManager::instance()->currentPlayList()->currentTrack() == track)
            MediaPlayer::instance()->stop();

        QString fileName = formatter->format(track); //generate file name

        QString ext = QStringLiteral(".") + track->path().section(QLatin1Char('.'), -1).toLower();
        if(!ext.isEmpty() && !fileName.endsWith(ext, Qt::CaseInsensitive))
            fileName += ext; //append extension
        //rename file
        QFile file(track->path());
        QString dest = QFileInfo(track->path()).absolutePath ();
        if(isValid(track) && file.rename(dest + QLatin1Char('/') + fileName) && isValid(track))
        {
            track->setPath(dest + QLatin1Char('/') + fileName);
            track->updateMetaData();
            model->doCurrentVisibleRequest();
        }
        else
            continue;
    }
}

void FileOps::move(const QList<PlayListTrack *> &tracks, const QString &dest, const MetaDataFormatter *formatter, PlayListModel *model)
{
    QProgressDialog progress(qApp->activeWindow ());
    progress.setWindowModality(Qt::WindowModal);
    progress.setWindowTitle(tr("Moving"));
    progress.setCancelButtonText(tr("Stop"));
    progress.show();
    progress.setAutoClose (false);
    int i  = 0;
    for(PlayListTrack *track : std::as_const(tracks))
    {
        if(!isValid(track) || !QFile::exists(track->path()))
            continue;

        if(PlayListManager::instance()->selectedPlayList() != model)
            break;

        if(PlayListManager::instance()->currentPlayList()->currentTrack() == track)
            MediaPlayer::instance()->stop();

        QString fileName = formatter->format(track); //generate file name

        QString ext = QStringLiteral(".") + track->path().section(QLatin1Char('.'), -1).toLower();
        if(!ext.isEmpty() && !fileName.endsWith(ext, Qt::CaseInsensitive))
            fileName += ext;  //append extension
        //create destination path
        QString path = dest + QLatin1Char('/') + fileName;
        //skip moved files
        if(path == track->path())
            continue;

        QDir dir = QFileInfo(path).dir();
        if(!dir.exists())
        {
            if(!dir.mkpath(dir.absolutePath()))
            {
                qCWarning(plugin, "unable to create directory");
                continue;
            }
        }

        progress.setRange(0, 100);
        progress.setValue(0);
        progress.setLabelText (QString(tr("Moving file %1/%2")).arg(++i).arg(tracks.size()));
        progress.update();
        if(!isValid(track) || PlayListManager::instance()->selectedPlayList() != model)
        {
            progress.setValue(100);
            continue;
        }

        //try to rename file first
        if(QFile::rename(track->path(), path))
        {
            progress.setValue(100);
            track->setPath(path);
            model->doCurrentVisibleRequest();
            continue;
        }
        //copy file
        QFile in(track->path());
        QFile out(path);
        if(!in.open(QIODevice::ReadOnly))
        {
            qCWarning(plugin) << in.errorString();
            continue;
        }
        if(!out.open(QIODevice::WriteOnly))
        {
            qCWarning(plugin) << out.errorString();
            continue;
        }

        progress.setMaximum(int(in.size() / COPY_BLOCK_SIZE));
        progress.setValue(0);
        progress.update();

        while (!in.atEnd ())
        {
            progress.wasCanceled ();
            out.write(in.read(COPY_BLOCK_SIZE));
            progress.setValue(int(out.size() / COPY_BLOCK_SIZE));
            qApp->processEvents();
        }

        in.close();

        if(!isValid(track) || PlayListManager::instance()->selectedPlayList() != model)
            continue;

        if(!QFile::remove(track->path()))
            qCWarning(plugin, "unable to remove file '%s'", qPrintable(track->path()));

        track->setPath(path);
        model->doCurrentVisibleRequest();

        if(progress.wasCanceled())
            break;
    }
    progress.close();
}

void FileOps::execute(const QList<PlayListTrack *> &tracks, const MetaDataFormatter *formatter, PlayListModel *model)
{
    for(PlayListTrack *track : std::as_const(tracks))
    {
        if(!isValid(track) || !QFile::exists(track->path())) //is it file?
            continue;

        if(PlayListManager::instance()->selectedPlayList() != model)
            break;

        QString command = formatter->format(track); //generate file name
        qCDebug(plugin, "exec command: %s", qPrintable(command));

#ifdef Q_OS_WIN
        QStringList args = { u"/C"_s, command };
        QProcess::startDetached(u"cmd.exe"_s, args);
#else
        QStringList args = { u"-c"_s, command };
        QProcess::startDetached(u"sh"_s, args);
#endif
    }
}

bool FileOps::isValid(PlayListTrack *track) const
{
    const QList<PlayListTrack*> tracks = PlayListManager::instance()->selectedPlayList()->selectedTracks();
    return tracks.contains(track);
}
