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

#include "xuistyle.h"

int XUiStyle::styleHint(StyleHint hint, const QStyleOption *option,
                        const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch(hint)
    {
    case SH_ToolTip_WakeUpDelay:
        return 250; //700 by default, too slow for icon-only controls
    case SH_ToolTip_FallAsleepDelay:
        return 0;   //moving between neighbouring buttons shows each at once
    default:
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}
