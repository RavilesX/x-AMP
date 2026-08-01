/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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
#include "statusiconcoverwidget.h"

StatusIconCoverWidget::StatusIconCoverWidget(QWidget *parent)
        : QWidget(parent)
{}

void StatusIconCoverWidget::setImage(const QImage &img)
{
    m_image = img;
    update();
}

void StatusIconCoverWidget::paintEvent(QPaintEvent *p)
{
    QPainter paint(this);
    if(!m_image.isNull())
        paint.drawImage(0,0, m_image.scaled(p->rect().size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}
