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
#ifndef SKINNEDTOGGLEBUTTON_H
#define SKINNEDTOGGLEBUTTON_H

#include "pixmapwidget.h"

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedToggleButton : public PixmapWidget
{
Q_OBJECT
public:
    SkinnedToggleButton(QWidget *parent, uint on_n, uint on_p, uint off_n, uint off_p);

    bool isChecked() const;

public slots:
    void setChecked(bool);

signals:
    void clicked(bool);

private slots:
    void updateSkin() override;

private:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

    bool m_cursorin, m_old_on;
    uint m_on_n, m_on_p, m_off_n, m_off_p;
    bool m_on = false;
};


#endif
