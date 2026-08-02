/***************************************************************************
 *   Copyright (C) 2026 by x-AMP contributors                              *
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
#include <qmmp/qmmpsettings.h>
#include "xuimainwindow.h"
#include "xuifactory.h"

UiProperties XUiFactory::properties() const
{
    UiProperties props;
    props.hasAbout = true;
    props.name = tr("x-AMP Interface");
    props.shortName = "xui"_L1;
    return props;
}

QObject *XUiFactory::create()
{
    //ten bands, matching the frequencies this interface labels
    QmmpSettings::instance()->readEqSettings(EqSettings::EQ_BANDS_10);
    return new XUiMainWindow();
}

void XUiFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About x-AMP Interface"),
                       tr("x-AMP Interface") + QChar::LineFeed +
                       tr("Written by: x-AMP contributors"));
}

QString XUiFactory::translation() const
{
    return QLatin1String(":/xui_plugin_");
}
