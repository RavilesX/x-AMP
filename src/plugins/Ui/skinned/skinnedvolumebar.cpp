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
#include <math.h>
#include "skin.h"
#include "skinnedbutton.h"
#include "skinnedmainwindow.h"
#include "skinnedvolumebar.h"


SkinnedVolumeBar::SkinnedVolumeBar(QWidget *parent) : PixmapWidget(parent)
{
    setPixmap(skin()->getVolumeBar(0));
    draw(false);
}

void SkinnedVolumeBar::mousePressEvent(QMouseEvent *e)
{
    m_moving = true;
    press_pos = e->position().x();
    if(m_pos < e->position().x() && e->position().x() < m_pos + skin()->scaled(11))
    {
        press_pos = e->position().x() - m_pos;
        emit sliderPressed();
    }
    else
    {
        m_value = convert(qMax(qMin(width() - skin()->scaled(18), qRound(e->position().x()) - skin()->scaled(6)), 0));
        press_pos = skin()->scaled(6);
        emit sliderPressed();
        if (m_value != m_old)
            emit sliderMoved(m_value);
    }
    draw();
}

void SkinnedVolumeBar::mouseMoveEvent (QMouseEvent *e)
{
    if(m_moving)
    {
        int po = e->position().x();
        po = po - press_pos;

        if(0 <= po && po <= width() - skin()->scaled(18))
        {
            m_value = convert(po);
            draw();
            emit sliderMoved(m_value);
        }
    }
}

void SkinnedVolumeBar::mouseReleaseEvent(QMouseEvent*)
{
    m_moving = false;
    draw(false);
    m_old = m_value;
    emit sliderReleased();
}

void SkinnedVolumeBar::setValue(int v)
{
    if (m_moving || m_max == 0)
        return;
    m_value = v;
    draw(false);
}

void SkinnedVolumeBar::setMax(int max)
{
    m_max = max;
    draw(false);
}

void SkinnedVolumeBar::updateSkin()
{
    resize(skin()->getVolumeBar(0).size());
    draw(false);
    setCursor(skin()->getCursor(Skin::CUR_VOLBAL));
}

void SkinnedVolumeBar::draw(bool pressed)
{
    int p=int(ceil(double(m_value - m_min) * (width() - skin()->scaled(18)) / (m_max-m_min)));
    m_pixmap = skin()->getVolumeBar(27 * (m_value-m_min) / (m_max-m_min));
    QPainter paint(&m_pixmap);
    if(pressed)
        paint.drawPixmap(p, skin()->scaled(1), skin()->getButton(Skin::BT_VOL_P));
    else
        paint.drawPixmap(p, skin()->scaled(1), skin()->getButton(Skin::BT_VOL_N));
    setPixmap(m_pixmap);
    m_pos = p;
}

int SkinnedVolumeBar::convert(int p)
{
    return int(ceil(double(m_max - m_min) * p / (width() - skin()->scaled(18)) + m_min));
}
