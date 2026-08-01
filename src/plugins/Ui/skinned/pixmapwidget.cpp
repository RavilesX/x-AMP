/***************************************************************************
 *   Copyright (C) 2006-2026 by Ilya Kotov                                 *
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
#include <QPixmap>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include "skin.h"
#include "pixmapwidget.h"

PixmapWidget::PixmapWidget(QWidget *parent)
        : QWidget(parent), m_skin(Skin::instance())
{
    connect(m_skin, &Skin::skinChanged, this, &PixmapWidget::updateSkin);
}

void PixmapWidget::setPixmap(const QPixmap &pixmap, bool fixed_size)
{
    m_pixmap = pixmap;
    if(fixed_size)
        setFixedSize(m_pixmap.size());
    else
        resize(m_pixmap.size());
    update();
}

void PixmapWidget::paintEvent (QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter paint(this);
    style()->drawItemPixmap(&paint, rect(), Qt::AlignCenter, m_pixmap);
}

void PixmapWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(event->button() & Qt::LeftButton)
        emit mouseClicked();
}
