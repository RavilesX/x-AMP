/***************************************************************************
 *   Copyright(C)  2007-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   Based on Promoe, an XMMS2 Client                                      *
 *   Copyright (C) 2005-2006 by XMMS2 Team                                 *
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

#include "skinnedbutton.h"
#include "skin.h"
#include <QMouseEvent>

SkinnedButton::SkinnedButton(QWidget *parent, uint normal, uint pressed, uint cursor)
        : PixmapWidget(parent),
          m_name_normal(normal),
          m_name_pressed(pressed),
          m_name_cursor(cursor)
{
    setON(false);
    setCursor(skin()->getCursor(m_name_cursor));
}

SkinnedButton::~SkinnedButton()
{}

void SkinnedButton::updateSkin()
{
    setPixmap(skin()->getButton(m_name_normal));
    setCursor(skin()->getCursor(m_name_cursor));
}

void SkinnedButton::setON(bool on)
{
    setPixmap(skin()->getButton(on ? m_name_pressed : m_name_normal));
}
void SkinnedButton::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton)
        return;

    setON(true);
    m_pressed = true;
    QWidget::mousePressEvent(e);
}

void SkinnedButton::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_pressed)
        return;

    m_pressed = false;
    if(rect().contains(e->pos()))
    {
        setON(false);
        emit clicked();
    }
}

void SkinnedButton::mouseMoveEvent(QMouseEvent *e)
{
    setON(m_pressed && rect().contains(e->pos()));
}
