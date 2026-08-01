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
#include <bs2b/bs2bversion.h>
#include <qmmp/qmmp.h>
#include "effectbs2bfactory.h"
#include "bs2bsettingsdialog.h"
#include "bs2bplugin.h"

EffectProperties EffectBs2bFactory::properties() const
{
    EffectProperties properties;
    properties.name = tr("BS2B Plugin");
    properties.shortName = "bs2b"_L1;
    properties.hasSettings = true;
    properties.hasAbout = true;
    return properties;
}

Effect *EffectBs2bFactory::create()
{
    return new Bs2bPlugin();
}

QDialog *EffectBs2bFactory::createSettings(QWidget *parent)
{
    return new Bs2bSettingsDialog(parent);
}

void EffectBs2bFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About BS2B Effect Plugin"),
                       u"<p>"_s +
                       tr("This is the Qmmp plugin version of Boris Mikhaylov's headphone DSP "
                          "effect \"Bauer stereophonic-to-binaural\", abbreviated bs2b.") +
                       u"</p><p>"_s +
                       tr("Visit %1 for more details").arg(u"<a href=\"https://bs2b.sourceforge.net/\">https://bs2b.sourceforge.net/</a>"_s) +
                       u"</p><p>"_s +
                       tr("Compiled against libbs2b-%1").arg(QLatin1StringView(BS2B_VERSION_STR)) +
                       u"</p><p>"_s +
                       tr("Developers:") + u"<br>"_s+
                       tr("Ilya Kotov &lt;forkotov02@ya.ru&gt;") + u"<br>"_s +
                       tr("Sebastian Pipping &lt;sebastian@pipping.org&gt;") + u"<p>"_s);
}

QString EffectBs2bFactory::translation() const
{
    return QLatin1String(":/bs2b_plugin_");
}
