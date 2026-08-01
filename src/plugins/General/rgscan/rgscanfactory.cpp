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

#include <QMessageBox>
#include "rgscanhelper.h"
#include "rgscanfactory.h"

GeneralProperties RGScanFactory::properties() const
{
    GeneralProperties properties;
    properties.name = tr("ReplayGain Scanner Plugin");
    properties.shortName = "rgscan"_L1;
    properties.hasAbout = true;
    properties.hasSettings = false;
    properties.visibilityControl = false;
    return properties;
}

QObject *RGScanFactory::create(QObject *parent)
{
    return new RGScanHelper(parent);
}

QDialog *RGScanFactory::createSettings(QWidget *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

void RGScanFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About ReplayGain Scanner Plugin"),
                       tr("ReplayGain Scanner Plugin for Qmmp") + QChar::LineFeed +
                       tr("This plugin scans audio files and gives information for volume normalization") + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>") + QChar::LineFeed +
                       tr("Based on source code by:") + QChar::LineFeed +
                       tr("David Robinson <David@Robinson.org>") + QChar::LineFeed +
                       tr("Glen Sawyer <mp3gain@hotmail.com>") + QChar::LineFeed +
                       tr("Frank Klemm"));

}

QString RGScanFactory::translation() const
{
    return QLatin1String(":/rgscan_plugin_");
}
