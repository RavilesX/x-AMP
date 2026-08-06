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

#ifndef XUIDOCK_H
#define XUIDOCK_H

#include <QList>
#include <QObject>
#include <QPoint>
#include <QRect>

class QWidget;

/*!
 * Magnetic docking between the interface's windows.
 *
 * Each window is a separate top level, and dragging one near another snaps
 * their edges together; windows already touching the main one are carried
 * along when it moves, so a docked stack behaves like a single window until
 * it is deliberately pulled apart.
 *
 * This requires positioning windows from the client side, which X11 allows
 * and Wayland does not. Where it is unavailable the windows still drag,
 * through the compositor, but without snapping -- see XUiWindow.
 */
class XUiDock : public QObject
{
    Q_OBJECT
public:
    static XUiDock *instance();

    /*! The window others snap to and that carries them along. */
    void setMainWindow(QWidget *window);
    void addWindow(QWidget *window);

    /*!
     * Moves \b window towards \b target, snapping it to its neighbours and to
     * the screen edges. Companions docked to the main window move with it.
     */
    void moveWindow(QWidget *window, const QPoint &target);

    /*! Records which windows are currently touching the main one. */
    void rememberDocked();

    /*! True where windows can be positioned by the client, i.e. not Wayland. */
    static bool canSnap();

private:
    explicit XUiDock(QObject *parent = nullptr) : QObject(parent) {}

    /*! \b pos adjusted so \b moving lands flush against \b fixed, if close. */
    QPoint snapToWindow(const QPoint &pos, QWidget *moving, QWidget *fixed) const;
    /*! \b window at \b pos together with whatever it carries. */
    QRect stackGeometry(QWidget *window, const QPoint &pos) const;
    /*! Top left of \b rect snapped to the screen edges and held on screen. */
    QPoint fitToScreen(const QRect &rect, QWidget *moving) const;

    QWidget *m_main = nullptr;
    QList<QWidget *> m_windows;
    //offset of each docked companion from the main window, held while it moves
    QList<QPoint> m_offsets;
    QList<bool> m_docked;
};

#endif
