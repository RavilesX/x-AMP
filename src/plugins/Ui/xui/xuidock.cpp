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

#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QWidget>
#include "xuidock.h"

namespace
{
    //how close an edge has to come before it jumps flush
    constexpr int SNAP = 12;
    //slack allowed when deciding whether two windows are already touching
    constexpr int TOUCHING = 2;

    bool overlapsVertically(const QRect &a, const QRect &b)
    {
        return a.top() <= b.bottom() && b.top() <= a.bottom();
    }

    bool overlapsHorizontally(const QRect &a, const QRect &b)
    {
        return a.left() <= b.right() && b.left() <= a.right();
    }

    //sharing an edge, and overlapping along the other axis
    bool touching(const QRect &a, const QRect &b)
    {
        const bool vertical = overlapsHorizontally(a, b)
                && (qAbs(b.top() - a.bottom()) <= TOUCHING + 1
                    || qAbs(a.top() - b.bottom()) <= TOUCHING + 1);
        const bool horizontal = overlapsVertically(a, b)
                && (qAbs(b.left() - a.right()) <= TOUCHING + 1
                    || qAbs(a.left() - b.right()) <= TOUCHING + 1);
        return vertical || horizontal;
    }
}

XUiDock *XUiDock::instance()
{
    static XUiDock *dock = new XUiDock;
    return dock;
}

bool XUiDock::canSnap()
{
    //Wayland forbids a client from placing its own windows, so there are no
    //coordinates to snap with; the drag goes to the compositor instead.
    //
    //x-AMP: asked the other way round until now -- the test named xcb, so
    //every platform that is not X11 lost snapping, Windows included, though
    //it places windows as freely as X11 does. What is being excluded is
    //Wayland, so that is what the test says.
    return QGuiApplication::platformName() != QLatin1String("wayland");
}

void XUiDock::setMainWindow(QWidget *window)
{
    m_main = window;
    addWindow(window);
}

void XUiDock::addWindow(QWidget *window)
{
    if(m_windows.contains(window))
        return;
    m_windows.append(window);
    m_offsets.append(QPoint());
    m_docked.append(false);
}

void XUiDock::rememberDocked()
{
    if(!m_main)
        return;
    const QRect anchor = m_main->frameGeometry();

    //A window docked to another docked window belongs to the stack too: with
    //the playlist under the equalizer, only the equalizer touches the player,
    //and comparing against the player alone left the playlist behind. So grow
    //the set until nothing else joins it.
    for(int i = 0; i < m_windows.size(); ++i)
        m_docked[i] = false;

    bool grew = true;
    while(grew)
    {
        grew = false;
        for(int i = 0; i < m_windows.size(); ++i)
        {
            QWidget *window = m_windows.at(i);
            if(m_docked.at(i) || window == m_main || !window->isVisible())
                continue;

            const QRect other = window->frameGeometry();
            bool joins = touching(anchor, other);
            for(int j = 0; !joins && j < m_windows.size(); ++j)
            {
                if(m_docked.at(j))
                    joins = touching(m_windows.at(j)->frameGeometry(), other);
            }
            if(!joins)
                continue;

            m_docked[i] = true;
            //offsets are all relative to the main window, which is what moves
            m_offsets[i] = other.topLeft() - anchor.topLeft();
            grew = true;
        }
    }
}

QPoint XUiDock::snapToWindow(const QPoint &pos, QWidget *moving, QWidget *fixed) const
{
    const QRect other = fixed->frameGeometry();
    const QSize size = moving->frameGeometry().size();
    QRect candidate(pos, size);
    QPoint result = pos;

    if(overlapsVertically(candidate, other))
    {
        if(qAbs(candidate.left() - other.right()) <= SNAP)
            result.setX(other.right() + 1);
        else if(qAbs(candidate.right() - other.left()) <= SNAP)
            result.setX(other.left() - size.width());
        else if(qAbs(candidate.left() - other.left()) <= SNAP)
            result.setX(other.left()); //flush sides, for a tidy column
    }

    candidate = QRect(result, size);
    if(overlapsHorizontally(candidate, other))
    {
        if(qAbs(candidate.top() - other.bottom()) <= SNAP)
            result.setY(other.bottom() + 1);
        else if(qAbs(candidate.bottom() - other.top()) <= SNAP)
            result.setY(other.top() - size.height());
        else if(qAbs(candidate.top() - other.top()) <= SNAP)
            result.setY(other.top());
    }
    return result;
}

QRect XUiDock::stackGeometry(QWidget *window, const QPoint &pos) const
{
    QRect rect(pos, window->frameGeometry().size());
    if(window != m_main)
        return rect;

    //everything carried along counts as part of the same body
    for(int i = 0; i < m_windows.size(); ++i)
    {
        if(!m_docked.at(i) || m_windows.at(i) == m_main)
            continue;
        rect = rect.united(QRect(pos + m_offsets.at(i),
                                 m_windows.at(i)->frameGeometry().size()));
    }
    return rect;
}

QPoint XUiDock::fitToScreen(const QRect &rect, QWidget *moving) const
{
    QScreen *screen = moving->screen() ? moving->screen() : QGuiApplication::primaryScreen();
    if(!screen)
        return rect.topLeft();

    const QRect available = screen->availableGeometry();
    QPoint result = rect.topLeft();

    if(qAbs(rect.left() - available.left()) <= SNAP)
        result.setX(available.left());
    else if(qAbs(rect.right() - available.right()) <= SNAP)
        result.setX(available.right() + 1 - rect.width());

    if(qAbs(rect.top() - available.top()) <= SNAP)
        result.setY(available.top());
    else if(qAbs(rect.bottom() - available.bottom()) <= SNAP)
        result.setY(available.bottom() + 1 - rect.height());

    //Then hold it inside the screen. Without this the window manager stopped
    //whichever window reached the edge first while the rest of the stack kept
    //going, and the stack came apart. A body larger than the screen keeps its
    //top left corner on instead.
    const int maxX = qMax(available.left(), available.right() + 1 - rect.width());
    const int maxY = qMax(available.top(), available.bottom() + 1 - rect.height());
    result.setX(qBound(available.left(), result.x(), maxX));
    result.setY(qBound(available.top(), result.y(), maxY));
    return result;
}

void XUiDock::moveWindow(QWidget *window, const QPoint &target)
{
    QPoint pos = target;

    //snap against every other visible window, then against the screen
    for(QWidget *other : std::as_const(m_windows))
    {
        if(other == window || !other->isVisible())
            continue;
        //a companion carried along is not something to snap to
        if(window == m_main && m_docked.value(m_windows.indexOf(other)))
            continue;
        pos = snapToWindow(pos, window, other);
    }
    //the docked stack is snapped and held on screen as one body, so no member
    //of it is stopped on its own
    const QRect stack = stackGeometry(window, pos);
    pos += fitToScreen(stack, window) - stack.topLeft();

    if(window == m_main)
    {
        //carry the docked companions, keeping the offset they had when the
        //drag started so the stack holds its shape
        for(int i = 0; i < m_windows.size(); ++i)
        {
            if(!m_docked.at(i) || m_windows.at(i) == m_main)
                continue;
            m_windows.at(i)->move(pos + m_offsets.at(i));
        }
    }
    window->move(pos);
}
