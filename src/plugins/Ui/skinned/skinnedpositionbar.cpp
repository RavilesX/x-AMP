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

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <math.h>
#include "skin.h"
#include "skinnedbutton.h"
#include "skinnedpositionbar.h"

SkinnedPositionBar::SkinnedPositionBar(QWidget *parent) : PixmapWidget(parent)
{
    setPixmap(skin()->getPosBar());
    draw(false);
    setCursor(skin()->getCursor(Skin::CUR_POSBAR));
}

void SkinnedPositionBar::mousePressEvent(QMouseEvent *e)
{
    if(m_max <= 0)
        return;
    m_moving = true;
    press_pos = e->position().x();
    if (m_pos < e->position().x() && e->position().x() < m_pos + skin()->scaled(29))
    {
        press_pos = e->position().x() - m_pos;
        emit sliderPressed();
    }
    else
    {
        m_value = convert(qMax(qMin(width() - skin()->scaled(30), qRound(e->position().x()) - skin()->scaled(15)), 0));
        press_pos = skin()->scaled(15);
        emit sliderPressed();
        if (m_value != m_old)
        {
            emit sliderMoved(m_value);

        }
    }
    draw();
}

void SkinnedPositionBar::mouseMoveEvent(QMouseEvent *e)
{
    if(m_moving)
    {
        qint64 po = e->position().x();
        po = po - press_pos;

        if (0<=po && po <= width() - skin()->scaled(30))
        {
            m_value = convert(po);
            draw();
            emit sliderMoved(m_value);
        }
    }
}

void SkinnedPositionBar::wheelEvent(QWheelEvent *e)
{
    if(m_max == 0)
        return;

    if(e->angleDelta().y() > 0)
        m_value += 5000;
    else
        m_value -= 5000;

    m_value = qBound(qint64(0), m_value, m_max);
    draw();
    e->accept();
    emit sliderReleased();
}

void SkinnedPositionBar::mouseReleaseEvent(QMouseEvent*)
{
    draw(false);
    if (m_value != m_old && m_max > 0)
        m_old = m_value;
    m_moving = false;
    if(m_max > 0)
        emit sliderReleased();
}

void SkinnedPositionBar::setValue(qint64 v)
{
    if (m_moving || m_max == 0)
        return;
    m_value = v;
    draw(false);
}

void SkinnedPositionBar::setMaximum(qint64 max)
{
    m_max = max;
    draw(false);
}

void SkinnedPositionBar::updateSkin()
{
    resize(skin()->getPosBar().size());
    draw(false);
    setCursor(skin()->getCursor(Skin::CUR_POSBAR));
}

void SkinnedPositionBar::draw(bool pressed)
{
    qint64 p = qint64(ceil(double(m_value-m_min) * (width() - skin()->scaled(30)) / (m_max-m_min)));
    m_pixmap = skin()->getPosBar();
    if (m_max > 0)
    {
        QPainter paint(&m_pixmap);
        if (pressed)
            paint.drawPixmap(p,0,skin()->getButton(Skin::BT_POSBAR_P));
        else
            paint.drawPixmap(p,0,skin()->getButton(Skin::BT_POSBAR_N));
    }
    setPixmap(m_pixmap);
    m_pos = p;
}

qint64 SkinnedPositionBar::convert(qint64 p)
{
    return qint64(ceil(double(m_max - m_min) * (p) / (width() - skin()->scaled(30)) + m_min));
}
