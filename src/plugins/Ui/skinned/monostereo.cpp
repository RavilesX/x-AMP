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
#include "skin.h"
#include "monostereo.h"

MonoStereo::MonoStereo(QWidget *parent)
      : PixmapWidget(parent)
{
   m_pixmap = QPixmap(skin()->scaled(54), skin()->scaled(12));
   setChannels(0);
}

void MonoStereo::setChannels(int c)
{
   m_channels = c;
   QPainter paint(&m_pixmap);
   if(c == 0)
   {
       paint.drawPixmap(0, 0, skin()->getMSPart(Skin::MONO_I));
       paint.drawPixmap(skin()->scaled(27), 0, skin()->getMSPart(Skin::STEREO_I));
   }
   else if(c == 1)
   {
       paint.drawPixmap(0, 0, skin()->getMSPart(Skin::MONO_A));
       paint.drawPixmap(skin()->scaled(27), 0, skin()->getMSPart(Skin::STEREO_I));
   }
   else if (c > 1)
   {
      paint.drawPixmap(0, 0, skin()->getMSPart(Skin::MONO_I));
      paint.drawPixmap(skin()->scaled(27), 0, skin()->getMSPart(Skin::STEREO_A));
   }
   setPixmap(m_pixmap);
}

void MonoStereo::updateSkin()
{
   m_pixmap = QPixmap(skin()->scaled(54), skin()->scaled(12));
   setChannels(m_channels);
}
