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
#include <QPaintEvent>
#include <QMouseEvent>

#include "skinnedplaylistcontrol.h"
#include "skin.h"

SkinnedPlaylistControl::SkinnedPlaylistControl(QWidget *parent) : PixmapWidget(parent)
{
    setPixmap(skin()->getPlPart(Skin::PL_CONTROL));
    m_ratio = skin()->ratio();
}

void SkinnedPlaylistControl::mouseReleaseEvent(QMouseEvent *me)
{
	QPoint pt = me->pos();
    if(QRect(4 * m_ratio, m_ratio, 7 * m_ratio, 7 * m_ratio).contains(pt))
		emit previousClicked();
    else if(QRect(12 * m_ratio, m_ratio, 7 * m_ratio, 7 * m_ratio).contains(pt))
		emit playClicked();
    else if(QRect(21 * m_ratio, m_ratio, 7 * m_ratio, 7 * m_ratio).contains(pt))
		emit pauseClicked();
    else if(QRect(31 * m_ratio, m_ratio, 7 * m_ratio,7 * m_ratio).contains(pt))
		emit stopClicked();
    else if(QRect(40 * m_ratio, m_ratio, 7 * m_ratio, 7 * m_ratio).contains(pt))
		emit nextClicked();
    else if(QRect(49 * m_ratio, m_ratio, 7 * m_ratio, 7 * m_ratio).contains(pt))
		emit ejectClicked();
}

void SkinnedPlaylistControl::updateSkin()
{
    setCursor(skin()->getCursor(Skin::CUR_PNORMAL));
    setPixmap(skin()->getPlPart(Skin::PL_CONTROL));
    m_ratio = skin()->ratio();
}
