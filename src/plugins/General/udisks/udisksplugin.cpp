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

#include <QtDBus>
#include <QActionGroup>
#include <QApplication>
#include <QStyle>
#include <qmmpui/uihelper.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistitem.h>
#include <qmmp/qmmp.h>
#include "udisksdevice.h"
#include "udisksmanager.h"
#include "udisksplugin.h"

UDisksPlugin::UDisksPlugin(QObject *parent) : QObject(parent)
{
    qDBusRegisterMetaType<QVariantMapMap>();
    qDBusRegisterMetaType<QByteArrayList>();

    m_manager = new UDisksManager(this);
    m_actions = new QActionGroup(this);
    connect(m_manager, &UDisksManager::deviceAdded, this, &UDisksPlugin::addDevice);
    connect(m_manager, &UDisksManager::deviceRemoved, this, &UDisksPlugin::removeDevice);
    connect(m_actions, &QActionGroup::triggered, this, &UDisksPlugin::processAction);
    //load settings
    QSettings settings;
    settings.beginGroup(u"UDisks"_s);
    m_detectCDA = settings.value(u"cda"_s, true).toBool();
    m_detectRemovable = settings.value(u"removable"_s, true).toBool();
    m_addTracks = false; //do not load tracks on startup
    m_addFiles = false;
    //find existing devices
    const QList<QDBusObjectPath> devs = m_manager->findAllDevices();
    for(const QDBusObjectPath &o : std::as_const(devs))
        addDevice(o);
    //load remaining settings
    m_addTracks = settings.value(u"add_tracks"_s, false).toBool();
    m_removeTracks = settings.value(u"remove_tracks"_s, false).toBool();
    m_addFiles = settings.value(u"add_files"_s, false).toBool();
    m_removeFiles = settings.value(u"remove_files"_s, false).toBool();
    settings.endGroup();
}

UDisksPlugin::~UDisksPlugin()
{
}

void UDisksPlugin::removeDevice(QDBusObjectPath o)
{
    QList<UDisksDevice *>::iterator it = m_devices.begin();
    while(it != m_devices.end())
    {
        if((*it)->objectPath() == o)
        {
            delete (*it);
            it = m_devices.erase(it);
            qCDebug(plugin, "removed device: \"%s\"", qPrintable(o.path()));
            updateActions();
            break;
        }

        ++it;
    }
}

void UDisksPlugin::addDevice(QDBusObjectPath o)
{
    for(const UDisksDevice *device : std::as_const(m_devices)) //is it already exists?
    {
        if (device->objectPath() == o)
            return;
    }
    UDisksDevice *device = new UDisksDevice(o, this);

    if(device->isRemovable()) //detect removable devices only
    {
        qCDebug(plugin, "added device: \"%s\"", qPrintable(o.path()));
        m_devices << device;
        updateActions();
        connect(device, & UDisksDevice::changed, this, &UDisksPlugin::updateActions);
    }
    else
        delete device;
}

void UDisksPlugin::updateActions()
{
    // add action for cd audio or mounted volume
    for(const UDisksDevice *device : std::as_const(m_devices))
    {
        QString dev_path;
        if (m_detectCDA && device->isAudio()) //cd audio
        {
            dev_path = u"cdda://"_s + device->deviceFile();
        }
        else if (m_detectRemovable && device->isMounted() &&
                 device->property(u"Size"_s).toLongLong() < 40000000000LL &&
                 (device->property(u"IdType"_s).toString() == "vfat"_L1 ||
                  device->property(u"IdType"_s).toString() == "iso9660"_L1 ||
                  device->property(u"IdType"_s).toString() == "udf"_L1 ||
                  device->property(u"IdType"_s).toString() == "ext2"_L1)) //mounted volume
        {
            dev_path = device->mountPoints().constFirst();
        }
        else
            continue;

        if (!findAction(dev_path))
        {
            QAction *action = new QAction(this);
            QString actionText;
            if (device->isAudio())
            {
                actionText = QString(tr("Add CD \"%1\"")).arg(device->deviceFile());
            }
            else
            {
                QString name = device->property(u"IdLabel"_s).toString();
                if (name.isEmpty())
                    name = dev_path;

                actionText = QString(tr("Add Volume \"%1\"")).arg(name);
            }

            if (device->isOptical())
            {
                if(device->property(u"IdType"_s).toString() == "iso9660"_L1)
                    action->setIcon(qApp->style()->standardIcon(QStyle::SP_DriveDVDIcon));
                else
                    action->setIcon(qApp->style()->standardIcon(QStyle::SP_DriveCDIcon));
            }
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
    for(QAction *action : m_actions->actions())
    {
        if (!findDevice(action))
        {
            qCDebug(plugin, "removed menu item: \"%s\"", qPrintable(action->data().toString()));
            m_actions->removeAction(action);
            UiHelper::instance()->removeAction(action);
            removePath(action->data().toString());
            action->deleteLater();
        }
    }
}

void UDisksPlugin::processAction(QAction *action)
{
    qCDebug(plugin, "action triggered: %s", qPrintable(action->data().toString()));
    QString path = action->data().toString();
    PlayListManager::instance()->selectedPlayList()->addPath(path);
}

QAction *UDisksPlugin::findAction(const QString &dev_path)
{
    for(QAction *action : m_actions->actions())
    {
        if (action->data().toString() == dev_path)
            return action;
    }
    return nullptr;
}

UDisksDevice *UDisksPlugin::findDevice(QAction *action)
{
    for(UDisksDevice *device : std::as_const(m_devices))
    {
        QString dev_path;
        if (device->isAudio())
        {
            dev_path = u"cdda://"_s + device->deviceFile();
            if (dev_path == action->data().toString())
                return device;
        }
        if (device->isMounted())
        {
            dev_path = device->mountPoints().constFirst();
            if (dev_path == action->data().toString())
                return device;
        }
    }
    return nullptr;
}

void UDisksPlugin::addPath(const QString &path)
{
    PlayListModel *model = PlayListManager::instance()->selectedPlayList();

    if(model->contains(path))
        return;

    if(path.startsWith(u"cdda://"_s) && m_addTracks)
    {
        PlayListManager::instance()->selectedPlayList()->addPath(path);
        return;
    }

    if(!path.startsWith(u"cdda://"_s) && m_addFiles)
        PlayListManager::instance()->selectedPlayList()->addPath(path);
}

void UDisksPlugin::removePath(const QString &path)
{
    if((path.startsWith(u"cdda://"_s) && !m_removeTracks) ||
            (!path.startsWith(u"cdda://"_s) && !m_removeFiles)) //process settings
        return;

    PlayListModel *model = PlayListManager::instance()->selectedPlayList();

    int i = 0;
    while(!model->isEmpty() && i < model->trackCount())
    {
        if(model->track(i)->path().startsWith(path))
            model->removeTrack(i);
        else
            ++i;
    }
}
