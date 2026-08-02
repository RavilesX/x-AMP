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

#ifndef XUIICONS_H
#define XUIICONS_H

#include <QColor>

class QIcon;
class QPainter;
class QRectF;

/*!
 * The interface's icon set, drawn in code.
 *
 * Deliberately not QIcon::fromTheme: that resolves against whatever icon
 * theme the desktop happens to use, so the same build would look different on
 * every machine. Deliberately not bundled SVGs either -- these glyphs are
 * simple geometry, and drawing them means no third-party assets to license,
 * free recolouring from XUi's palette, and exact results at any device pixel
 * ratio.
 *
 * Every icon is drawn to fill \b rect, so the caller controls the size.
 */
namespace XUiIcons
{
    enum Icon
    {
        Menu,       //hamburger
        Minimize,
        Maximize,
        Close,
        Play,
        Pause,
        Stop,
        Previous,
        Next,
        Shuffle,
        Repeat,
        RepeatOne,
        Volume,
        VolumeMuted,
        Heart,
        More,       //horizontal ellipsis
        Kebab,      //vertical ellipsis
        Equalizer,
        PlaylistGlyph,
        Plus,
        Minus,
        SelectAll,
        Search,
        List,
        Settings,
        MusicNote,
        ChevronDown,
        ChevronRight,
    };

    /*!
     * Paints \b icon inside \b rect using \b color. Antialiasing is enabled
     * by the function and the painter state is restored on return.
     */
    void paint(QPainter *painter, Icon icon, const QRectF &rect, const QColor &color);

    /*!
     * Renders \b icon into a QIcon of \b size pixels, for the Qt widgets that
     * ask for one instead of painting themselves -- config dialog pages, for
     * instance. Honours the device pixel ratio.
     */
    QIcon toIcon(Icon icon, int size, const QColor &color);
}

#endif
