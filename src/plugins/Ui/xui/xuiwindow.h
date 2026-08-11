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

#ifndef XUIWINDOW_H
#define XUIWINDOW_H

#include <QPoint>
#include <QString>
#include <QWidget>

/*!
 * A frameless top-level window of this interface.
 *
 * Carries what every one of them needs: the rounded background, resizing by
 * the edges, dragging by a nominated handle, and geometry remembered under a
 * name. Dragging goes through XUiDock so windows snap to each other; where
 * the client cannot place its own windows the drag is handed to the
 * compositor instead and only the snapping is lost.
 */
class XUiWindow : public QWidget
{
    Q_OBJECT
public:
    /*!
     * \b key names this window's geometry in the settings, so each one is
     * restored where the user left it.
     */
    explicit XUiWindow(const QString &key, QWidget *parent = nullptr);

    /*!
     * Marks the area that drags the window. Presses anywhere else fall
     * through to the widgets inside.
     */
    void setDragHandle(QWidget *handle);

    void saveGeometry();
    /*!
     * Restores position and size, clamping the result onto a screen.
     * Returns false when nothing was saved and only \b fallback was applied,
     * so the caller can place the window itself.
     */
    bool restoreGeometry(const QSize &fallback);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    //carries the docked companions to wherever this window really
    //ended up, which the window manager has the last word on
    void moveEvent(QMoveEvent *) override;
    void closeEvent(QCloseEvent *) override;

private:
    /*! Which edges the pointer is over, for the resize cursors. */
    Qt::Edges edgesAt(const QPoint &pos) const;
    bool onHandle(const QPoint &pos) const;

    QString m_key;
    QWidget *m_handle = nullptr;
    //offset from the window's corner to the pointer, held during a drag
    QPoint m_grab;
    bool m_dragging = false;
};

#endif
