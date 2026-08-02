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
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
#include "skin.h"
#include "skinnedeqslider.h"

SkinnedEqSlider::SkinnedEqSlider(QWidget *parent): PixmapWidget(parent)
{
    setPixmap(skin()->getEqSlider(0));
    draw(false);
    setCursor(skin()->getCursor(Skin::CUR_EQSLID));
}

void SkinnedEqSlider::mousePressEvent(QMouseEvent *e)
{
    m_moving = true;
    press_pos = e->position().y();
    if(e->button() == Qt::MiddleButton)
    {
        m_value = 0;
        emit sliderMoved(m_value);
        m_old = m_value;
    }
    else if(m_pos<e->position().y() && e->position().y() < m_pos + skin()->scaled(11))
    {
        press_pos = e->position().y() - m_pos;
    }
    else
    {
        m_value = convert(qMax(qMin(height() - skin()->scaled(12), qRound(e->position().y()) - skin()->scaled(6)), 0));
        press_pos = skin()->scaled(6);
        if(m_value != m_old)
        {
            emit sliderMoved(m_value);
            m_old = m_value;
        }
    }
    draw();
}

void SkinnedEqSlider::mouseReleaseEvent(QMouseEvent*)
{
    m_moving = false;
    draw(false);
}

void SkinnedEqSlider::mouseMoveEvent(QMouseEvent *e)
{
    if(m_moving)
    {
        int po = e->position().y() - press_pos;

        if(0 <= po && po <= height() - skin()->scaled(12))
        {
            m_value = convert(po);
            draw();
            if(m_value!=m_old)
            {

                m_old = m_value;
                emit sliderMoved(-m_value);
            }
        }
    }
}

double SkinnedEqSlider::value()
{
    return - m_value;
}

void SkinnedEqSlider::setValue(double p)
{
    if(m_moving)
        return;
    m_value = -p;
    draw(false);
}

void SkinnedEqSlider::setMax(double m)
{
    m_max = m;
    draw(false);
}

void SkinnedEqSlider::updateSkin()
{
    resize(skin()->getEqSlider(0).size());
    draw(false);
    setCursor(skin()->getCursor(Skin::CUR_EQSLID));
}

void SkinnedEqSlider::draw(bool pressed)
{
    //x-AMP: compose at base scale, then scale once (see SkinnedVolumeBar)
    QPixmap composed = skin()->getEqSliderBase(27 - 27 * (m_value-m_min) / (m_max-m_min));
    int p = int(std::ceil(double(m_value - m_min) * (composed.height() - 12) / (m_max-m_min)));
    {
        QPainter paint(&composed);
        paint.drawPixmap(1, p, skin()->getButtonBase(pressed ? Skin::EQ_BT_BAR_P : Skin::EQ_BT_BAR_N));
    }
    m_pixmap = skin()->scalePixmap(composed);
    setPixmap(m_pixmap);
    m_pos = skin()->scaled(p); //hit testing happens in widget coordinates
}

double SkinnedEqSlider::convert(int p)
{
    return (m_max - m_min) * p / (height() - skin()->scaled(12)) + m_min;
}

void SkinnedEqSlider::wheelEvent(QWheelEvent *e)
{
    m_value -= double(e->angleDelta().y()) / 60;
    m_value = m_value > m_max ? m_max : m_value;
    m_value = m_value < m_min ? m_min : m_value;
    draw(false);
    emit sliderMoved(m_value);
}
