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
#include <cmath>
#include "skin.h"
#include "skinnedbutton.h"
#include "skinnedmainwindow.h"
#include "skinnedbalancebar.h"

SkinnedBalanceBar::SkinnedBalanceBar(QWidget *parent)
        : PixmapWidget(parent)
{
    setPixmap(skin()->getBalanceBar(0));
    draw(false);
}

SkinnedBalanceBar::~SkinnedBalanceBar()
{}

int SkinnedBalanceBar::value() const
{
    return m_value;
}

void SkinnedBalanceBar::mousePressEvent(QMouseEvent *e)
{
    m_moving = true;
    press_pos = e->position().x();
    if(e->button() == Qt::MiddleButton)
    {
        m_value = 0;
        emit sliderPressed();
        emit sliderMoved(m_value);
    }
    else if(m_pos<e->position().x() && e->position().x()<m_pos+11*skin()->ratio())
    {
        press_pos = e->position().x()-m_pos;
        emit sliderPressed();
    }
    else
    {
        m_value = convert(qMax(qMin(width() - 18 * skin()->ratio(), qRound(e->position().x()) - 6 * skin()->ratio()),0));
        press_pos = 6 * skin()->ratio();
        emit sliderPressed();
        if (m_value != m_old)
        {
            emit sliderMoved(m_value);
        }
    }
    draw();
}

void SkinnedBalanceBar::mouseMoveEvent(QMouseEvent *e)
{
    if(m_moving)
    {
        int po = e->position().x();
        po = po - press_pos;

        if(0 <= po && po <= width() - 13 * skin()->ratio())
        {
            m_value = convert(po);
            draw();
            emit sliderMoved(m_value);
        }
    }
}

void SkinnedBalanceBar::mouseReleaseEvent(QMouseEvent*)
{
    m_moving = false;
    draw(false);
    m_old = m_value;
    emit sliderReleased();
}

void SkinnedBalanceBar::setValue(int v)
{
    if(m_moving || m_max == 0)
        return;
    m_value = v;
    draw(false);
}

void SkinnedBalanceBar::setMax(int max)
{
    m_max = max;
    draw(false);
}

void SkinnedBalanceBar::updateSkin()
{
    resize(skin()->getBalanceBar(0).size());
    draw(false);
}

void SkinnedBalanceBar::draw(bool pressed)
{
    if(std::abs(m_value) < 6)
        m_value = 0;
    int p=int(ceil(double(m_value - m_min) * (width() - 13 * skin()->ratio()) / (m_max-m_min)));
    m_pixmap = skin()->getBalanceBar(std::abs(27 * m_value / m_max));
    QPainter paint(&m_pixmap);
    if(pressed)
        paint.drawPixmap(p, skin()->ratio(), skin()->getButton(Skin::BT_BAL_P));
    else
        paint.drawPixmap(p, skin()->ratio(), skin()->getButton(Skin::BT_BAL_N));
    setPixmap(m_pixmap);
    m_pos = p;
}

int SkinnedBalanceBar::convert(int p)
{
    return int(ceil(double(m_max - m_min) * p / (width() - 13 * skin()->ratio()) + m_min));
}
