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
#ifndef SKINNEDEQTITLEBAR_H
#define SKINNEDEQTITLEBAR_H

#include "pixmapwidget.h"

class QMouseEvent;
class SkinnedMainWindow;
class SkinnedButton;
class ShadedBar;
class SkinnedEqWidget;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedEqTitleBar : public PixmapWidget
{
Q_OBJECT
public:
    SkinnedEqTitleBar(SkinnedEqWidget *parent);

    ~SkinnedEqTitleBar();

    void setActive(bool);

private slots:
    void shade();
    void updateSkin() override;

private:
    void updatePositions();
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

    QPoint m_pos;
    SkinnedEqWidget *m_eq;
    SkinnedMainWindow *m_mw;
    SkinnedButton *m_close;
    SkinnedButton *m_shade;
    SkinnedButton *m_shade2 = nullptr;
    bool m_shaded = false, m_align = false;
    ShadedBar *m_volumeBar = nullptr;
    ShadedBar *m_balanceBar = nullptr;
};

#endif
