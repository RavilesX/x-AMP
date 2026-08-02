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

#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <cmath>
#include "xuiicons.h"

namespace
{
    constexpr qreal PI = 3.14159265358979323846;

    //Every glyph below is designed on this square and then scaled to the
    //caller's rect, so the shapes stay proportional at any size.
    constexpr qreal GRID = 24.0;

    void strokePath(QPainter *p, const QPainterPath &path, const QColor &c, qreal width)
    {
        QPen pen(c, width);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p->strokePath(path, pen);
    }

    QPainterPath line(qreal x1, qreal y1, qreal x2, qreal y2)
    {
        QPainterPath path;
        path.moveTo(x1, y1);
        path.lineTo(x2, y2);
        return path;
    }

    void dots(QPainter *p, const QColor &c, bool vertical)
    {
        p->setBrush(c);
        p->setPen(Qt::NoPen);
        for(int i = -1; i <= 1; ++i)
        {
            const qreal off = 12.0 + i * 5.5;
            p->drawEllipse(QPointF(vertical ? 12.0 : off, vertical ? off : 12.0), 1.5, 1.5);
        }
    }

    //speaker body shared by the volume icons
    QPainterPath speaker()
    {
        QPainterPath path;
        path.moveTo(4, 9);
        path.lineTo(7.5, 9);
        path.lineTo(12, 5);
        path.lineTo(12, 19);
        path.lineTo(7.5, 15);
        path.lineTo(4, 15);
        path.closeSubpath();
        return path;
    }

    //triangle pointing right, used by play and the skip buttons
    QPainterPath triangle(qreal left, qreal right, qreal top, qreal bottom)
    {
        QPainterPath path;
        path.moveTo(left, top);
        path.lineTo(right, (top + bottom) / 2.0);
        path.lineTo(left, bottom);
        path.closeSubpath();
        return path;
    }

    void arrowHead(QPainterPath *path, qreal x, qreal y, qreal dx, qreal dy)
    {
        path->moveTo(x - dx, y - dy);
        path->lineTo(x, y);
        path->lineTo(x - dy, y + dx);
    }
}

void XUiIcons::paint(QPainter *p, Icon icon, const QRectF &rect, const QColor &color)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->translate(rect.topLeft());
    p->scale(rect.width() / GRID, rect.height() / GRID);

    //stroke widths are in grid units, so they scale with the icon
    constexpr qreal W = 1.8;

    switch(icon)
    {
    case Menu:
    {
        QPainterPath path;
        for(int i = 0; i < 3; ++i)
            path.addPath(line(4, 7.0 + i * 5.0, 20, 7.0 + i * 5.0));
        strokePath(p, path, color, W);
        break;
    }
    case Minimize:
        strokePath(p, line(5, 12, 19, 12), color, W);
        break;
    case Maximize:
    {
        QPainterPath path;
        path.addRoundedRect(QRectF(5.5, 5.5, 13, 13), 2, 2);
        strokePath(p, path, color, W);
        break;
    }
    case Close:
    {
        QPainterPath path;
        path.addPath(line(6.5, 6.5, 17.5, 17.5));
        path.addPath(line(17.5, 6.5, 6.5, 17.5));
        strokePath(p, path, color, W);
        break;
    }
    case Play:
        p->setPen(Qt::NoPen);
        p->setBrush(color);
        p->drawPath(triangle(8.5, 18.5, 5, 19));
        break;
    case Pause:
    {
        p->setPen(Qt::NoPen);
        p->setBrush(color);
        QPainterPath path;
        path.addRoundedRect(QRectF(7.5, 5, 3.6, 14), 1.4, 1.4);
        path.addRoundedRect(QRectF(12.9, 5, 3.6, 14), 1.4, 1.4);
        p->drawPath(path);
        break;
    }
    case Stop:
        p->setPen(Qt::NoPen);
        p->setBrush(color);
        {
            QPainterPath path;
            path.addRoundedRect(QRectF(6, 6, 12, 12), 1.8, 1.8);
            p->drawPath(path);
        }
        break;
    case Previous:
    case Next:
    {
        p->setPen(Qt::NoPen);
        p->setBrush(color);
        if(icon == Previous)
        {
            p->translate(GRID, 0);
            p->scale(-1, 1);
        }
        QPainterPath path = triangle(6.5, 16.5, 5.5, 18.5);
        path.addRoundedRect(QRectF(16.8, 5.5, 2.6, 13), 1.2, 1.2);
        p->drawPath(path);
        break;
    }
    case Shuffle:
    {
        QPainterPath path;
        path.moveTo(3.5, 7);
        path.lineTo(7.5, 7);
        path.cubicTo(11, 7, 13, 17, 16.5, 17);
        path.lineTo(20, 17);
        path.moveTo(3.5, 17);
        path.lineTo(7.5, 17);
        path.cubicTo(11, 17, 13, 7, 16.5, 7);
        path.lineTo(20, 7);
        arrowHead(&path, 20.5, 7, 3, 3);
        arrowHead(&path, 20.5, 17, 3, 3);
        strokePath(p, path, color, W);
        break;
    }
    case Repeat:
    case RepeatOne:
    {
        QPainterPath path;
        path.addRoundedRect(QRectF(4, 6.5, 16, 11), 4.5, 4.5);
        strokePath(p, path, color, W);
        //arrow sitting on the top edge, marking the direction of the loop
        QPainterPath head;
        head.moveTo(12.5, 3.6);
        head.lineTo(15.4, 6.5);
        head.lineTo(12.5, 9.4);
        strokePath(p, head, color, W);
        if(icon == RepeatOne)
        {
            //a numeral 1 inside the loop: repeat this track, not the list
            QPainterPath one;
            one.moveTo(10.4, 10.6);
            one.lineTo(12.1, 9.4);
            one.lineTo(12.1, 14.8);
            strokePath(p, one, color, 1.7);
        }
        break;
    }
    case Volume:
    case VolumeMuted:
    {
        p->setPen(Qt::NoPen);
        p->setBrush(color);
        p->drawPath(speaker());
        if(icon == Volume)
        {
            QPainterPath waves;
            waves.arcMoveTo(QRectF(9, 7.5, 9, 9), 60);
            waves.arcTo(QRectF(9, 7.5, 9, 9), 60, -120);
            waves.arcMoveTo(QRectF(7.5, 4.5, 15, 15), 55);
            waves.arcTo(QRectF(7.5, 4.5, 15, 15), 55, -110);
            strokePath(p, waves, color, 1.6);
        }
        else
        {
            QPainterPath cross;
            cross.addPath(line(15, 9.5, 20.5, 15));
            cross.addPath(line(20.5, 9.5, 15, 15));
            strokePath(p, cross, color, 1.6);
        }
        break;
    }
    case Heart:
    {
        QPainterPath path;
        path.moveTo(12, 19.5);
        path.cubicTo(2.5, 13.5, 5, 5, 12, 8.6);
        path.cubicTo(19, 5, 21.5, 13.5, 12, 19.5);
        strokePath(p, path, color, W);
        break;
    }
    case More:
        dots(p, color, false);
        break;
    case Kebab:
        dots(p, color, true);
        break;
    case Equalizer:
    {
        QPainterPath path;
        const qreal xs[3] = { 6.5, 12, 17.5 };
        const qreal knobs[3] = { 9.5, 15, 7.5 };
        for(int i = 0; i < 3; ++i)
            path.addPath(line(xs[i], 4, xs[i], 20));
        strokePath(p, path, color, 1.5);
        p->setPen(Qt::NoPen);
        p->setBrush(color);
        for(int i = 0; i < 3; ++i)
            p->drawRoundedRect(QRectF(xs[i] - 3, knobs[i] - 1.4, 6, 2.8), 1.4, 1.4);
        break;
    }
    case PlaylistGlyph:
    case List:
    {
        QPainterPath path;
        const qreal right = icon == List ? 20.0 : 15.0;
        for(int i = 0; i < 3; ++i)
            path.addPath(line(4, 6.5 + i * 5.5, right, 6.5 + i * 5.5));
        strokePath(p, path, color, W);
        if(icon == PlaylistGlyph)
        {
            //small note hanging off the last line
            p->setPen(Qt::NoPen);
            p->setBrush(color);
            p->drawEllipse(QPointF(17, 18), 2.4, 2.0);
            strokePath(p, line(19.4, 18, 19.4, 10.5), color, 1.5);
        }
        break;
    }
    case Plus:
    {
        QPainterPath path;
        path.addPath(line(12, 5, 12, 19));
        path.addPath(line(5, 12, 19, 12));
        strokePath(p, path, color, W);
        break;
    }
    case Search:
    {
        QPainterPath path;
        path.addEllipse(QPointF(10.5, 10.5), 5.5, 5.5);
        path.addPath(line(14.6, 14.6, 19.5, 19.5));
        strokePath(p, path, color, W);
        break;
    }
    case Settings:
    {
        QPainterPath path;
        path.addEllipse(QPointF(12, 12), 3.4, 3.4);
        strokePath(p, path, color, W);
        QPainterPath teeth;
        for(int i = 0; i < 8; ++i)
        {
            const qreal a = i * PI / 4.0;
            teeth.addPath(line(12 + std::cos(a) * 6.0, 12 + std::sin(a) * 6.0,
                               12 + std::cos(a) * 8.6, 12 + std::sin(a) * 8.6));
        }
        strokePath(p, teeth, color, 2.0);
        break;
    }
    case MusicNote:
    {
        p->setPen(Qt::NoPen);
        p->setBrush(color);
        p->drawEllipse(QPointF(8, 17.5), 3.6, 3.0);
        p->drawEllipse(QPointF(17.5, 15.5), 3.6, 3.0);
        QPainterPath stems;
        stems.addPath(line(11.4, 17.5, 11.4, 6.5));
        stems.addPath(line(20.9, 15.5, 20.9, 4.5));
        strokePath(p, stems, color, 1.8);
        QPainterPath beam;
        beam.moveTo(11.4, 6.5);
        beam.lineTo(20.9, 4.5);
        strokePath(p, beam, color, 2.6);
        break;
    }
    case ChevronDown:
    case ChevronRight:
    {
        QPainterPath path;
        if(icon == ChevronDown)
        {
            path.moveTo(7, 10);
            path.lineTo(12, 15);
            path.lineTo(17, 10);
        }
        else
        {
            path.moveTo(10, 7);
            path.lineTo(15, 12);
            path.lineTo(10, 17);
        }
        strokePath(p, path, color, W);
        break;
    }
    }

    p->restore();
}
