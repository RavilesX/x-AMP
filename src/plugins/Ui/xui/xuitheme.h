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

#ifndef XUITHEME_H
#define XUITHEME_H

#include <QColor>

/*!
 * Colours and metrics for the whole interface.
 *
 * Everything is drawn with QPainter against these values instead of being
 * loaded from bitmaps, so the interface stays sharp at any size and any
 * device pixel ratio. Put new constants here rather than hard-coding colours
 * in the widgets.
 */
namespace XUi
{
    //surfaces, darkest first
    inline const QColor Background = QColor(0x08, 0x0b, 0x10);
    inline const QColor Card       = QColor(0x0d, 0x11, 0x18);
    inline const QColor CardTop    = QColor(0x11, 0x16, 0x1f); //card gradient start
    inline const QColor Elevated   = QColor(0x16, 0x1c, 0x27); //chips, inset panels
    inline const QColor Border     = QColor(0x1c, 0x23, 0x30);
    inline const QColor Hover      = QColor(0x1f, 0x27, 0x35);

    //accent
    inline const QColor Accent       = QColor(0x2f, 0x86, 0xf6);
    inline const QColor AccentBright = QColor(0x6f, 0xb4, 0xff);
    inline const QColor AccentDeep   = QColor(0x0d, 0x47, 0xc4);

    //text
    inline const QColor Text      = QColor(0xe9, 0xee, 0xf6);
    inline const QColor TextDim   = QColor(0x8b, 0x96, 0xa8);
    inline const QColor TextFaint = QColor(0x4d, 0x57, 0x67);

    //Metrics. Kept deliberately tight: three stacked cards add up fast, and
    //the window's minimum size is derived from them, so every pixel here is
    //a pixel the user cannot shrink the window below.
    inline constexpr int CardRadius       = 12;
    inline constexpr int CardGap          = 10;
    inline constexpr int CardPadding      = 14;
    inline constexpr int WindowRadius     = 14;
    inline constexpr int TitleBarHeight   = 38;
    inline constexpr int CardHeaderHeight = 46;
}

#endif
