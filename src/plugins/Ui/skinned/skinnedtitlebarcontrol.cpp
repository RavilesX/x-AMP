/***************************************************************************
 *   Copyright (C) 2007-2026 by Ilya Kotov                                 *
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
#include "skin.h"
#include "skinnedtitlebarcontrol.h"

SkinnedTitleBarControl::SkinnedTitleBarControl(QWidget *parent) : QWidget(parent)
{
    //setAutoFillBackground(true);
    m_ratio = Skin::instance()->ratio();
    resize(Skin::instance()->scaled(57), Skin::instance()->scaled(10));
    connect(Skin::instance(), &Skin::skinChanged, this, &SkinnedTitleBarControl::updateSkin);
}

void SkinnedTitleBarControl::mousePressEvent(QMouseEvent *)
{}

void SkinnedTitleBarControl::mouseReleaseEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();
    if(QRect(0, 0, Skin::instance()->scaled(8), Skin::instance()->scaled(10)).contains(pt))
        emit previousClicked();
    else if(QRect(Skin::instance()->scaled(8), 0, Skin::instance()->scaled(11), Skin::instance()->scaled(10)).contains(pt))
        emit playClicked();
    else if(QRect(Skin::instance()->scaled(19), 0, Skin::instance()->scaled(10), Skin::instance()->scaled(10)).contains(pt))
        emit pauseClicked();
    else if(QRect(Skin::instance()->scaled(29), 0, Skin::instance()->scaled(8), Skin::instance()->scaled(10)).contains(pt))
        emit stopClicked();
    else if(QRect(Skin::instance()->scaled(37), 0, Skin::instance()->scaled(10), Skin::instance()->scaled(10)).contains(pt))
        emit nextClicked();
    else if(QRect(Skin::instance()->scaled(47), 0 ,Skin::instance()->scaled(10), Skin::instance()->scaled(10)).contains(pt))
        emit ejectClicked();
}

void SkinnedTitleBarControl::mouseMoveEvent(QMouseEvent*)
{}

void SkinnedTitleBarControl::updateSkin()
{
    m_ratio = Skin::instance()->ratio();
    resize(Skin::instance()->scaled(57), Skin::instance()->scaled(10));
}
