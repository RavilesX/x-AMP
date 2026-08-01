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

#include <QMessageBox>
#include <qmmp/qmmp.h>
#include "effectstereofactory.h"
#include "stereosettingsdialog.h"
#include "stereoplugin.h"

EffectProperties EffectStereoFactory::properties() const
{
    EffectProperties properties;
    properties.name = tr("Extra Stereo Plugin");
    properties.shortName = "stereo"_L1;
    properties.hasSettings = true;
    properties.hasAbout = true;
    return properties;
}

Effect *EffectStereoFactory::create()
{
    return new StereoPlugin();
}

QDialog *EffectStereoFactory::createSettings(QWidget *parent)
{
    return new StereoSettingsDialog(parent);
}

void EffectStereoFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About Extra Stereo Plugin"),
                       tr("x-AMP Extra Stereo Plugin") + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>") + QChar::LineFeed +
                       tr("Based on the Extra Stereo Plugin for Xmms by Johan Levin"));
}

QString EffectStereoFactory::translation() const
{
    return QLatin1String(":/stereo_plugin_");
}
