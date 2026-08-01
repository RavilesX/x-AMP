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
#ifndef SKINNEDEQSLIDER_H
#define SKINNEDEQSLIDER_H

#include "pixmapwidget.h"

class QMouseEvent;
class QWheelEvent;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedEqSlider : public PixmapWidget
{
Q_OBJECT
public:
    SkinnedEqSlider(QWidget *parent = nullptr);

    ~SkinnedEqSlider() = default;

    double value();

public slots:
    void setValue(double);
    void setMax(double);

signals:
    void sliderMoved (double);

private slots:
    void updateSkin() override;

private:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent *) override;

    bool m_moving = false;
    int press_pos;
    double m_max = 20, m_min = -20, m_pos = 0, m_value = 0, m_old = 0;
    QPixmap m_pixmap;
    double convert(int);   // value = convert(position);
    void draw(bool pressed = true);
};

#endif
