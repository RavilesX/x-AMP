/***************************************************************************
 *   Copyright (C) 2007-2026 by Ilya Kotov                                 *
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
#include "alsasettingsdialog.h"
#include "outputalsa.h"
#include "outputalsafactory.h"


OutputProperties OutputALSAFactory::properties() const
{
    OutputProperties properties;
    properties.name = tr("ALSA Plugin");
    properties.shortName = "alsa"_L1;
    properties.hasAbout = true;
    properties.hasSettings = true;
    return properties;
}

Output* OutputALSAFactory::create()
{
    return new OutputALSA();
}

Volume *OutputALSAFactory::createVolume()
{
    //x-AMP: a mixer that could not be opened must not be handed back as if it
    //had. The card and element are settings that default to hw:0 and PCM, and
    //hw:0 is whichever card the kernel enumerated first -- plugging in a
    //monitor with HDMI audio is enough to make that the wrong one. When the
    //element is missing, setVolume() returns without doing anything and
    //volume() reports zero, yet VolumeHandler still reads the object as a
    //working hardware control and stops applying volume to the audio itself.
    //The result is a volume control that moves and does nothing at all.
    //Returning nothing instead lets it fall back to software volume.
    VolumeALSA *volume = new VolumeALSA();
    if(volume->isValid())
        return volume;

    qCWarning(plugin, "no usable mixer element, falling back to software volume");
    delete volume;
    return nullptr;
}

QDialog *OutputALSAFactory::createSettings(QWidget *parent)
{
    return new AlsaSettingsDialog(parent);
}

void OutputALSAFactory::showAbout(QWidget *parent)
{
    QMessageBox::about (parent, tr("About ALSA Output Plugin"),
                        tr("x-AMP ALSA Output Plugin") + QChar::LineFeed +
                        tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QString OutputALSAFactory::translation() const
{
    return QLatin1String(":/alsa_plugin_");
}
