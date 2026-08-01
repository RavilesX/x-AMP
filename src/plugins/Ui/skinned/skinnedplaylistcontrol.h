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
 
#ifndef _PALYLISTCONTROL_H
#define _PALYLISTCONTROL_H

#include "pixmapwidget.h"

class QMouseEvent;
class PaintEvent;

 /**
    @author Vladimir Kuznetsov <vovanec@gmail.ru>
 */
class SkinnedPlaylistControl : public PixmapWidget
{
Q_OBJECT
public:
    SkinnedPlaylistControl(QWidget* parent = nullptr);


signals:
	void previousClicked();
	void nextClicked();
	void pauseClicked();
	void playClicked();
	void stopClicked();
	void ejectClicked();

private slots:
    void updateSkin() override;

private:
    void mouseReleaseEvent(QMouseEvent*) override;
    int m_ratio;
};

#endif 
