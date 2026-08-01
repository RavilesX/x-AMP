/***************************************************************************
 *   Copyright (C) 2017-2026 by Ilya Kotov                                 *
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

#include <QMessageBox>
#include <QtPlugin>
#include <QApplication>
#include <qmmp/qmmp.h>
#include <shout/shout.h>
#include "shoutclient.h"
#include "shoutoutput.h"
#include "shoutsettingsdialog.h"
#include "outputshoutfactory.h"

OutputShoutFactory::OutputShoutFactory()
{
    m_connection = new ShoutClient(qApp);
}

OutputProperties OutputShoutFactory::properties() const
{
    OutputProperties properties;
    properties.name = tr("Icecast Plugin");
    properties.hasAbout = true;
    properties.hasSettings = true;
    properties.shortName = "shout"_L1;
    return properties;
}

Output* OutputShoutFactory::create()
{
    return new ShoutOutput(m_connection);
}

Volume *OutputShoutFactory::createVolume()
{
    return nullptr;
}

QDialog *OutputShoutFactory::createSettings(QWidget *parent)
{
    return new ShoutSettingsDialog(parent);
}

void OutputShoutFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About Icecast Output Plugin"),
                       tr("Qmmp Icecast Output Plugin") + QChar::LineFeed +
                       tr("Compiled against libshout-%1").arg(QString::fromLatin1(shout_version(nullptr,nullptr,nullptr))) +
                       QChar::LineFeed + tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QString OutputShoutFactory::translation() const
{
    return QLatin1String(":/shout_plugin_");
}
