/***************************************************************************
 *   Copyright (C) 2011-2026 by Ilya Kotov                                 *
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
#include <QWheelEvent>
#include <QStyleOptionSlider>
#include <QStyle>
#include <QToolTip>
#include <qmmpui/metadataformatter.h>
#include "qsuipositionslider.h"

QSUiPositionSlider::QSUiPositionSlider(QWidget *parent) : QSlider(Qt::Horizontal, parent)
{
    connect(this, &QSUiPositionSlider::sliderMoved, this, &QSUiPositionSlider::onSliderMoved);
}

void QSUiPositionSlider::mousePressEvent (QMouseEvent *event)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    if(event->button() == Qt::LeftButton && !sr.contains(event->pos()))
    {
        int val;
        if(orientation() == Qt::Vertical)
            val = minimum() + ((maximum() - minimum()) * (height() - event->position().y())) / height();
        else if(layoutDirection() == Qt::RightToLeft)
            val = maximum() - ((maximum() - minimum()) * event->position().x()) / width();
        else
            val = minimum() + ((maximum() - minimum()) * event->position().x()) / width();

        if(invertedAppearance())
        {
            setValue(maximum() - val);
            onSliderMoved(maximum() - val);
        }
        else
        {
            setValue(val);
            onSliderMoved(val);
        }

        setSliderDown (true);
        event->accept();
    }
    QSlider::mousePressEvent(event);
}

void QSUiPositionSlider::mouseReleaseEvent (QMouseEvent *event)
{
    setSliderDown (false);
    QSlider::mouseReleaseEvent(event);
}

void QSUiPositionSlider::wheelEvent(QWheelEvent *event)
{
    setValue(value() + event->angleDelta().y() / 20);
    emit sliderReleased();
}

void QSUiPositionSlider::onSliderMoved(int pos)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    rect.moveTo(rect.x() - 10 , rect.y() - 45);
    QToolTip::showText(mapToGlobal(rect.topLeft()), MetaDataFormatter::formatDuration(pos * 1000), this, QRect());
}
