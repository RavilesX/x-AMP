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
#ifndef SKINNEDPOSITIONBAR_H
#define SKINNEDPOSITIONBAR_H

#include "pixmapwidget.h"

class QMouseEvent;
class QWheelEvent;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedPositionBar : public PixmapWidget
{
    Q_OBJECT
public:
    SkinnedPositionBar(QWidget *parent = nullptr);

    inline qint64 value() const { return m_value; }
    inline qint64 maximum() const { return m_max; }

public slots:
    void setValue(qint64);
    void setMaximum(qint64);

signals:
    void sliderMoved (qint64);
    void sliderPressed();
    void sliderReleased();

private slots:
    void updateSkin() override;

private:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent *e) override;
    bool m_moving = false;
    qint64 press_pos;
    qint64 m_max = 0, m_min = 0, m_pos, m_value = 0, m_old = 0;
    QPixmap m_pixmap;
    qint64 convert(qint64);   // value = convert(position);
    void draw(bool pressed = true);
};

#endif
