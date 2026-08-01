/***************************************************************************
 *   Copyright (C) 2018-2026 by Ilya Kotov                                 *
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
#include <QStorageInfo>
#include <QActionGroup>
#include <QStyle>
#include <QDir>
#include <QSettings>
#include <qmmpui/playlistmanager.h>
#include <windows.h>
#include <dbt.h>
#include <qmmpui/uihelper.h>
#include "removablehelper.h"

RemovableHelper::RemovableHelper(QObject *parent): QObject(parent)
{
    qApp->installNativeEventFilter(this);
    m_actions = new QActionGroup(this);
    connect(m_actions, &QActionGroup::triggered, this, &RemovableHelper::processAction);
    //load settings
    QSettings settings;
    settings.beginGroup("rdetect"_L1);
    m_detectCDA = settings.value("cda"_L1, true).toBool();
    m_detectRemovable = settings.value("removable"_L1, true).toBool();
    m_addTracks = false; //do not load tracks on startup
    m_addFiles = false;
    //find existing devices
    updateActions();
    //load remaining settings
    m_addTracks = settings.value("add_tracks"_L1, false).toBool();
    m_removeTracks = settings.value("remove_tracks"_L1, false).toBool();
    m_addFiles = settings.value("add_files"_L1, false).toBool();
    m_removeFiles = settings.value("remove_files"_L1, false).toBool();
    settings.endGroup();

}

RemovableHelper::~RemovableHelper()
{
    qApp->removeNativeEventFilter(this);
}

bool RemovableHelper::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);

    if(eventType == "windows_generic_MSG")
    {
        MSG *msg = static_cast<MSG *>(message);
        if(msg->message == WM_DEVICECHANGE && msg->wParam == DBT_DEVICEARRIVAL)
        {
            PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;

            if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                updateActions();
                return true;
            }
        }
        else if(msg->message == WM_DEVICECHANGE && msg->wParam == DBT_DEVICEREMOVECOMPLETE)
        {
            PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;

            if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                updateActions();
                return true;
            }
        }
    }
    return false;
}

void RemovableHelper::processAction(QAction *action)
{
    qCDebug(plugin, "action triggered: %s", qPrintable(action->data().toString()));
    QString path = action->data().toString();
    PlayListManager::instance()->selectedPlayList()->addPath(path);
}

void RemovableHelper::updateActions()
{
    const QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();

    for(const QStorageInfo &storage : std::as_const(volumes))
    {
        if(!storage.isValid() || !storage.isReady())
            continue;

        QString dev_path;

        if(m_detectRemovable && storage.bytesTotal() < 40000000000LL &&
                (storage.fileSystemType() == "NTFS" ||
                 storage.fileSystemType() == "FAT32" ||
                 storage.fileSystemType() == "UDF"))
        {
            dev_path = storage.rootPath();
        }
        else if(storage.fileSystemType() == "CDFS")
        {
            dev_path = storage.rootPath();
            if(isAudioCd(dev_path))
            {
                dev_path = QStringLiteral("cdda://%1").arg(dev_path);
                dev_path = dev_path.left(dev_path.size() - 1); //remove trailing '/'
            }
        }
        else
            continue;

        if(!findAction(dev_path))
        {
            QAction *action = new QAction(this);
            QString actionText;
            if(dev_path.startsWith("cdda"_L1))
            {
                actionText = tr("Add CD \"%1\"").arg(storage.displayName());
            }
            else
            {
                actionText = tr("Add Volume \"%1\"").arg(storage.displayName());
            }


            if(storage.fileSystemType() == "CDFS")
                action->setIcon(qApp->style()->standardIcon(QStyle::SP_DriveCDIcon));
            else if(storage.fileSystemType() == "UDF")
                action->setIcon(qApp->style()->standardIcon(QStyle::SP_DriveDVDIcon));
            else
                action->setIcon(qApp->style()->standardIcon(QStyle::SP_DriveHDIcon));
            qCDebug(plugin, "added menu item: \"%s\"", qPrintable(dev_path));

            action->setText(actionText);
            action->setData(dev_path);
            m_actions->addAction(action);
            UiHelper::instance()->addAction(action, UiHelper::ADD_MENU);
            addPath(dev_path);
        }
    }
    // remove action if device is unmounted/removed
    const QList<QAction *> actions = m_actions->actions();
    for(QAction *action : std::as_const(actions))
    {
        bool found = false;

        for(const QStorageInfo &storage : std::as_const(volumes))
        {
            QString dev_path = storage.rootPath();
            if(isAudioCd(dev_path))
            {
                dev_path = QStringLiteral("cdda://%1").arg(dev_path);
                dev_path = dev_path.left(dev_path.size() - 1); //remove trailing '/'
            }

            if(dev_path == action->data().toString())
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            qCDebug(plugin, "removed menu item: \"%s\"", qPrintable(action->data().toString()));
            m_actions->removeAction(action);
            UiHelper::instance()->removeAction(action);
            removePath(action->data().toString());
            action->deleteLater();
        }
    }
}

QAction *RemovableHelper::findAction(const QString &dev_path)
{
    for(QAction *action : m_actions->actions())
    {
        if(action->data().toString() == dev_path)
            return action;
    }
    return nullptr;
}

void RemovableHelper::addPath(const QString &path)
{
    PlayListModel *model = PlayListManager::instance()->selectedPlayList();

    for(PlayListTrack *track : model->tracks()) // Is it already exist?
    {
        if(track->path().startsWith(path))
            return;
    }

    if(path.startsWith(u"cdda://"_s) && m_addTracks)
    {
        model->addPath(path);
        return;
    }
    else if(!path.startsWith(u"cdda://"_s) && m_addFiles)
        model->addPath(path);
}

void RemovableHelper::removePath(const QString &path)
{
    if((path.startsWith(u"cdda://"_s) && !m_removeTracks) ||
            (!path.startsWith(u"cdda://"_s) && !m_removeFiles)) //process settings
        return;

    PlayListModel *model = PlayListManager::instance()->selectedPlayList();

    int i = 0;
    while(model->trackCount() > 0 && i < model->trackCount())
    {
        if(model->track(i)->path().startsWith(path))
            model->removeTrack(i);
        else
            ++i;
    }
}

bool RemovableHelper::isAudioCd(const QString &path)
{
    QDir dir(path);
    return !dir.entryInfoList({ u"*.cda"_s }).isEmpty();
}

