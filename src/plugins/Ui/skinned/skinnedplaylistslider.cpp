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
#include <QPainter>
#include <QResizeEvent>
#include <math.h>

#include "skin.h"
#include "skinnedplaylistslider.h"
#include "pixmapwidget.h"

SkinnedPlayListSlider::SkinnedPlayListSlider(QWidget *parent)
        : QWidget(parent)
{
    m_skin = Skin::instance();
    connect(m_skin, &Skin::skinChanged, this, &SkinnedPlayListSlider::updateSkin);
    setCursor(m_skin->getCursor(Skin::CUR_PVSCROLL));
}

SkinnedPlayListSlider::~SkinnedPlayListSlider()
{}

void SkinnedPlayListSlider::paintEvent(QPaintEvent *)
{
    int sy = (height() - 58) / 29;
    int p = int(ceil(double(m_value - m_min) * (height() - 18) / (m_max - m_min)));
    QPainter paint(this);
    paint.drawPixmap(0, 0, m_skin->getPlPart(Skin::PL_RFILL));
    paint.drawPixmap(0, 29, m_skin->getPlPart(Skin::PL_RFILL));

    for(int i = 0; i < sy; i++)
    {
        paint.drawPixmap(0, 58 + i * 29, m_skin->getPlPart(Skin::PL_RFILL));
    }
    if(m_pressed)
        paint.drawPixmap(5 * m_skin->ratio(), p, m_skin->getButton(Skin::PL_BT_SCROLL_P));
    else
        paint.drawPixmap(5 * m_skin->ratio(), p, m_skin->getButton(Skin::PL_BT_SCROLL_N));
    m_pos = p;
}

void SkinnedPlayListSlider::mousePressEvent(QMouseEvent *e)
{
    m_moving = true;
    m_pressed = true;
    m_press_pos = e->position().y();
    if(m_pos < e->position().y() && e->position().y() < m_pos + 18 * m_skin->ratio())
    {
        m_press_pos = e->position().y() - m_pos;
    }
    else
    {
        int value = convert(qMax(qMin(height() - 18 * m_skin->ratio(), qRound(e->position().y()) - 9 * m_skin->ratio()), 0));
        m_press_pos = 9 * m_skin->ratio();
        if(m_value != value)
        {
            emit sliderMoveRequest(value);
        }
    }
    update();
}

void SkinnedPlayListSlider::mouseReleaseEvent(QMouseEvent*)
{
    m_moving = false;
    m_pressed = false;
    update();
}

void SkinnedPlayListSlider::mouseMoveEvent(QMouseEvent* e)
{
    if(m_moving)
    {
        int po = e->position().y();
        po = po - m_press_pos;

        if(0 <= po && po <= height() - 18 * m_skin->ratio())
        {
            int value = convert(po);

            if(m_value != value)
                emit sliderMoveRequest(value);

            update();
        }
    }
}

void SkinnedPlayListSlider::setPos(int p, int max)
{
    m_max = max;
    m_value = p;
    update();
}

void SkinnedPlayListSlider::updateSkin()
{
    update();
    setCursor(m_skin->getCursor(Skin::CUR_PVSCROLL));
}

int SkinnedPlayListSlider::convert(int p)
{
    return int(floor(double(m_max - m_min) * (p) / (height() - 18 * m_skin->ratio()) + m_min));
}
