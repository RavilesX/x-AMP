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

#include <QAction>
#include <QSettings>
#include <QApplication>
#include <QProgressDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <qmmp/soundcore.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistitem.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/metadataformatter.h>
#include "batchtageditordialog.h"
#include "batchtageditor.h"

BatchTagEditor::BatchTagEditor(QObject *parent) : QObject(parent)
{
    m_pl_manager = PlayListManager::instance();
    //actions
    QAction *editTagsAction = new QAction(tr("Change Tags"), this);
    //register all actions
    connect(editTagsAction, &QAction::triggered, this, &BatchTagEditor::editTags);
    UiHelper::instance()->addAction(editTagsAction, UiHelper::PLAYLIST_MENU);
}

BatchTagEditor::~BatchTagEditor()
{}

void BatchTagEditor::editTags()
{
    qCDebug(plugin) << Q_FUNC_INFO;
    QStringList paths;
    const QList<PlayListTrack *> tracks = m_pl_manager->selectedPlayList()->selectedTracks();
    for(PlayListTrack *t : std::as_const(tracks))
    {
#ifdef Q_OS_WIN
        //skip current playing track
        if(SoundCore::instance()->trackInfo().path() == t->path())
            continue;
#endif

        if(!t->path().contains(QStringLiteral("://")) && QFileInfo::exists(t->path()))
            paths << t->path();
    }

    if(paths.isEmpty())
    {
        QMessageBox::warning(qApp->activeWindow(), tr("Warning"), tr("Unable to find editable tracks"));
        return;
    }

    BatchTagEditorDialog dialog(qApp->activeWindow());
    dialog.setFiles(paths);
    if(dialog.exec() == QDialog::Accepted)
    {
        const QList<PlayListTrack *> tracks = m_pl_manager->selectedPlayList()->selectedTracks();
        for(PlayListTrack *t : std::as_const(tracks))
            t->updateMetaData();
        m_pl_manager->selectedPlayList()->updateMetaData();
    }
}
