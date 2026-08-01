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

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QXmlStreamReader>
#include <qmmp/qmmp.h>
#include "udisksmanager.h"

using namespace Qt::Literals::StringLiterals;

UDisksManager::UDisksManager(QObject *parent)
        : QObject(parent)
{
    m_interface = new QDBusInterface(u"org.freedesktop.UDisks2"_s, u"/org/freedesktop/UDisks2"_s,
                                     u"org.freedesktop.DBus.ObjectManager"_s,
                                     QDBusConnection::systemBus(), this);

    m_interface->connection().connect(u"org.freedesktop.UDisks2"_s, u"/org/freedesktop/UDisks2"_s,
                                      u"org.freedesktop.DBus.ObjectManager"_s, u"InterfacesAdded"_s,
                                      this, SLOT(onInterfacesAdded(QDBusObjectPath,QVariantMapMap)));

    m_interface->connection().connect(u"org.freedesktop.UDisks2"_s, u"/org/freedesktop/UDisks2"_s,
                                      u"org.freedesktop.DBus.ObjectManager"_s, u"InterfacesRemoved"_s,
                                      this, SIGNAL(onInterfacesRemoved(QDBusObjectPath,QStringList)));
}


UDisksManager::~UDisksManager()
{
}

QList<QDBusObjectPath> UDisksManager::findAllDevices()
{
    QList<QDBusObjectPath> paths;
    QDBusMessage call = QDBusMessage::createMethodCall(u"org.freedesktop.UDisks2"_s,
                                                       u"/org/freedesktop/UDisks2/block_devices"_s,
                                                       u"org.freedesktop.DBus.Introspectable"_s,
                                                       u"Introspect"_s);
    QDBusPendingReply<QString> reply = QDBusConnection::systemBus().call(call);


    if (!reply.isValid())
    {
        qCWarning(plugin, "error: %s", qPrintable(reply.error().name()));
        return paths;
    }
    QXmlStreamReader xml(reply.value());
    while (!xml.atEnd())
    {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name().toString() == "node"_L1 )
        {
            QString name = xml.attributes().value("name"_L1).toString();
            if(!name.isEmpty())
                paths << QDBusObjectPath(u"/org/freedesktop/UDisks2/block_devices/"_s + name);
        }
    }
    return paths;
}

void UDisksManager::onInterfacesAdded(const QDBusObjectPath &object_path, const QVariantMapMap &)
{
    if(object_path.path().startsWith(u"/org/freedesktop/UDisks2/jobs"_s))
        return;
    emit deviceAdded(object_path);
}

void UDisksManager::onInterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &)
{
    if(object_path.path().startsWith(u"/org/freedesktop/UDisks2/jobs"_s))
        return;
    emit deviceRemoved(object_path);
}
