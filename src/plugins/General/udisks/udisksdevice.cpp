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
#include <QByteArray>
#include <QDBusMetaType>
#include <qmmp/qmmp.h>
#include "udisksdevice.h"

using namespace Qt::Literals::StringLiterals;

UDisksDevice::UDisksDevice(QDBusObjectPath o, QObject *parent) : QObject(parent)
{
    m_block_interface = new QDBusInterface(u"org.freedesktop.UDisks2"_s, o.path(),
                                           u"org.freedesktop.UDisks2.Block"_s, QDBusConnection::systemBus(),
                                           this);

    QDBusObjectPath drive_object = property(u"Drive"_s).value<QDBusObjectPath>();

    QDBusConnection::systemBus().connect(u"org.freedesktop.UDisks2"_s, o.path(),
                                         u"org.freedesktop.DBus.Properties"_s, u"PropertiesChanged"_s,
                                         this, SIGNAL(changed()));

    m_drive_interface = new QDBusInterface(u"org.freedesktop.UDisks2"_s, drive_object.path(),
                                           u"org.freedesktop.UDisks2.Drive"_s, QDBusConnection::systemBus(),
                                           this);
    m_path = o;
}

UDisksDevice::~UDisksDevice()
{
}

QVariant UDisksDevice::property(const QString &key) const
{
    return m_block_interface->property(key.toLatin1().data());
}

bool UDisksDevice::isRemovable() const
{
    return m_drive_interface->property("Removable").toBool();
}

bool UDisksDevice::isMediaRemovable() const
{
    return m_drive_interface->property("MediaRemovable").toBool();
}

bool UDisksDevice::isAudio() const
{
    return m_drive_interface->property("OpticalNumAudioTracks").toInt() > 0;
}

bool UDisksDevice::isMounted() const
{
    return !mountPoints().isEmpty();
}

bool UDisksDevice::isOptical() const
{
    return m_drive_interface->property("Optical").toBool();
}

QStringList UDisksDevice::mountPoints() const
{
    QStringList points;
    QDBusMessage message = QDBusMessage::createMethodCall(u"org.freedesktop.UDisks2"_s, m_path.path(),
                                                          u"org.freedesktop.DBus.Properties"_s, u"Get"_s);

    QList<QVariant> arguments;
    arguments << u"org.freedesktop.UDisks2.Filesystem"_s << u"MountPoints"_s;
    message.setArguments(arguments);

    QDBusMessage reply = QDBusConnection::systemBus().call(message);

    const QList<QVariant> args = reply.arguments();

    for(const QVariant &arg : std::as_const(args))
    {
        QByteArrayList list;
        QDBusArgument a = arg.value<QDBusVariant>().variant().value<QDBusArgument>();
        if(a.currentType() != QDBusArgument::ArrayType)
            continue;
        a >> list;

        for(const QByteArray &p : std::as_const(list))
            points.append(QString::fromLocal8Bit(p));
    }
    return points;
}

QString UDisksDevice::deviceFile() const
{
    return QString::fromLatin1(m_block_interface->property("Device").toByteArray());
}

QDBusObjectPath UDisksDevice::objectPath() const
{
    return m_path;
}
