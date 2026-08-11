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

#include <QCloseEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QSettings>
#include <QWindow>
#include <qmmp/qmmp.h>
#include "xuitheme.h"
#include "xuidock.h"
#include "xuiwindow.h"

namespace
{
    //grab area along the border for resizing
    constexpr int RESIZE_MARGIN = 6;
}

XUiWindow::XUiWindow(const QString &key, QWidget *parent)
    : QWidget(parent), m_key(key)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); //rounded corners need this
    setMouseTracking(true);
    XUiDock::instance()->addWindow(this);
}

void XUiWindow::setDragHandle(QWidget *handle)
{
    m_handle = handle;
    if(m_handle)
        m_handle->setMouseTracking(true);
}

bool XUiWindow::onHandle(const QPoint &pos) const
{
    //the handle need not be a direct child -- a card's own header bar sits a
    //couple of levels down -- so map rather than compare geometries
    if(!m_handle)
        return false;

    const QPoint local = m_handle->mapFrom(this, pos);
    if(!m_handle->rect().contains(local))
        return false;

    //A press on a control inside the bar belongs to the control, not to the
    //drag. Under X11 the omission went unnoticed, because the drag is tracked
    //here and Qt still delivers the release to the widget; everywhere else the
    //fallback is startSystemMove(), which hands the gesture to the window
    //manager, and the release never arrives. That left every switch and button
    //living in a header dead on Windows -- the equalizer's controls, the
    //playlist's search field, and the main window's own menu and close.
    //Decorative labels are marked transparent for mouse events, so the bar can
    //still be dragged by its title.
    return m_handle->childAt(local) == nullptr;
}

Qt::Edges XUiWindow::edgesAt(const QPoint &pos) const
{
    if(isMaximized())
        return {};
    Qt::Edges edges;
    if(pos.x() <= RESIZE_MARGIN)
        edges |= Qt::LeftEdge;
    if(pos.x() >= width() - RESIZE_MARGIN)
        edges |= Qt::RightEdge;
    if(pos.y() <= RESIZE_MARGIN)
        edges |= Qt::TopEdge;
    if(pos.y() >= height() - RESIZE_MARGIN)
        edges |= Qt::BottomEdge;
    return edges;
}

void XUiWindow::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton)
        return;

    const Qt::Edges edges = edgesAt(e->pos());
    if(edges)
    {
        windowHandle()->startSystemResize(edges);
        return;
    }
    if(!onHandle(e->pos()))
        return;

    //TEMPORARY, to be removed once the Windows package is understood
    qCWarning(plugin, "x-AMP diagnostic: press on handle, platform=%s, canSnap=%d",
              qPrintable(QGuiApplication::platformName()), int(XUiDock::canSnap()));

    if(XUiDock::canSnap())
    {
        //Track the drag ourselves, which is the only way to compute snapping:
        //startSystemMove hands the whole gesture to the compositor and reports
        //nothing back. Note where the companions sit before anything moves.
        m_dragging = true;
        m_grab = e->globalPosition().toPoint() - frameGeometry().topLeft();
        XUiDock::instance()->rememberDocked();
    }
    else
    {
        windowHandle()->startSystemMove(); //no snapping, but it still drags
    }
}

void XUiWindow::mouseMoveEvent(QMouseEvent *e)
{
    if(m_dragging)
    {
        XUiDock::instance()->moveWindow(this, e->globalPosition().toPoint() - m_grab);
        return;
    }

    const Qt::Edges edges = edgesAt(e->pos());
    if((edges & Qt::LeftEdge && edges & Qt::TopEdge) ||
       (edges & Qt::RightEdge && edges & Qt::BottomEdge))
        setCursor(Qt::SizeFDiagCursor);
    else if((edges & Qt::RightEdge && edges & Qt::TopEdge) ||
            (edges & Qt::LeftEdge && edges & Qt::BottomEdge))
        setCursor(Qt::SizeBDiagCursor);
    else if(edges & (Qt::LeftEdge | Qt::RightEdge))
        setCursor(Qt::SizeHorCursor);
    else if(edges & (Qt::TopEdge | Qt::BottomEdge))
        setCursor(Qt::SizeVerCursor);
    else
        unsetCursor();
}

void XUiWindow::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}

void XUiWindow::leaveEvent(QEvent *)
{
    //Qt sends Leave when the pointer moves onto a child too. Without this a
    //resize cursor picked up at the edge stays set, and every child without a
    //cursor of its own inherits it.
    unsetCursor();
}

void XUiWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(XUi::Border, 1));
    p.setBrush(XUi::Background);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                      XUi::WindowRadius, XUi::WindowRadius);
}

void XUiWindow::closeEvent(QCloseEvent *e)
{
    //Nothing here decides that the card was put away: the application closes
    //its windows on the way out too, and that must not untick anything.
    saveGeometry();
    e->accept();
}

void XUiWindow::saveGeometry()
{
    if(isMaximized() || !isVisible())
        return; //a maximised frame is not what should come back
    QSettings settings;
    settings.setValue(QStringLiteral("XUi/%1_geometry").arg(m_key), geometry());
}

bool XUiWindow::restoreGeometry(const QSize &fallback)
{
    QSettings settings;
    const QRect saved = settings.value(QStringLiteral("XUi/%1_geometry").arg(m_key))
                        .toRect();
    if(!saved.isValid())
    {
        resize(fallback);
        return false;
    }

    resize(saved.size().expandedTo(minimumSizeHint()));

    //A window saved on a monitor that is no longer there would open off
    //screen, where it cannot be dragged back.
    QScreen *screen = QGuiApplication::screenAt(saved.topLeft());
    if(!screen)
        screen = QGuiApplication::primaryScreen();
    if(screen)
    {
        const QRect available = screen->availableGeometry();
        QPoint pos = saved.topLeft();
        pos.setX(qBound(available.left(), pos.x(), available.right() - width()));
        pos.setY(qBound(available.top(), pos.y(), available.bottom() - height()));
        move(pos);
    }
    return true;
}
