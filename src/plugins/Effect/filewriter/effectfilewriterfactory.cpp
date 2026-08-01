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
#include <qmmp/qmmp.h>
#include "filewriterplugin.h"
#include "filewritersettingsdialog.h"
#include "effectfilewriterfactory.h"

EffectProperties EffectFileWriterFactory::properties() const
{
    EffectProperties properties;
    properties.name = tr("File Writer Plugin");
    properties.shortName = "filewriter"_L1;
    properties.hasSettings = true;
    properties.hasAbout = true;
    properties.priority = EffectProperties::EFFECT_PRIORITY_LOW;
    return properties;
}

Effect *EffectFileWriterFactory::create()
{
    return new FileWriterPlugin();
}

QDialog *EffectFileWriterFactory::createSettings(QWidget *parent)
{
    return new FileWriterSettingsDialog(parent);
}

void EffectFileWriterFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About File Writer Plugin"),
                       tr("Qmmp File Writer Plugin") + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QString EffectFileWriterFactory::translation() const
{
    return QLatin1String(":/filewriter_plugin_");
}
