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
#ifndef PIXMAPWIDGET_H
#define PIXMAPWIDGET_H

#include <QWidget>

class Skin;
class QPixmap;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class PixmapWidget : public QWidget
{
Q_OBJECT
public:
    PixmapWidget(QWidget *parent = nullptr);

    virtual ~PixmapWidget() = default;
    virtual void setPixmap(const QPixmap &pixmap, bool fixed_size = false);

protected slots:
    virtual void updateSkin() = 0;

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    inline const Skin *skin() const { return m_skin; };

signals:
    void mouseClicked();

private:
    QPixmap m_pixmap;
    Skin *m_skin;
};

#endif
