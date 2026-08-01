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
#include <QSettings>
#include <QMouseEvent>
#include <qmmp/qmmp.h>
#include "skin.h"
#include "skinnedtimeindicator.h"

SkinnedTimeIndicatorModel::SkinnedTimeIndicatorModel(QObject *parent) : QObject (parent)
{
    readSettings();
}

SkinnedTimeIndicatorModel::~SkinnedTimeIndicatorModel()
{
    writeSettings();
}

void SkinnedTimeIndicatorModel::setPosition(int position)
{
    if(m_position != position)
    {
        m_position = position;
        emit changed();
    }
}

void SkinnedTimeIndicatorModel::setDuration(int duration)
{
    if(m_duration != duration)
    {
        m_duration = duration;
        emit changed();
    }
}

void SkinnedTimeIndicatorModel::setElapsed(bool elapsed)
{
    if(m_elapsed != elapsed)
    {
        m_elapsed = elapsed;
        emit changed();
    }
}

void SkinnedTimeIndicatorModel::setVisible(bool visible)
{
    if(m_visible != visible)
    {
        m_visible = visible;
        emit changed();
    }
}

int SkinnedTimeIndicatorModel::displayTime()
{
    int t = m_position;

    if(t < 0)
    {
        return 0;
    }

    if(!m_elapsed)
    {
        t = m_position - m_duration;
    }

    if(qAbs(t) >= 3600)
    {
        t /= 60;
    }

    return t;
}

void SkinnedTimeIndicatorModel::toggleElapsed()
{
    setElapsed(!elapsed());
}

void SkinnedTimeIndicatorModel::readSettings()
{
    QSettings settings;
    m_elapsed = settings.value("Skinned/disp_elapsed"_L1, true).toBool();
}

void SkinnedTimeIndicatorModel::writeSettings()
{
    QSettings settings;
    settings.setValue("Skinned/disp_elapsed"_L1, m_elapsed);
}


SkinnedTimeIndicator::SkinnedTimeIndicator(SkinnedTimeIndicatorModel *model, QWidget *parent)
    : PixmapWidget (parent)
    , m_model (model)
{
    SkinnedTimeIndicator::updateSkin();
    connect(m_model, &SkinnedTimeIndicatorModel::changed, this, &SkinnedTimeIndicator::modelChanged);
}

void SkinnedTimeIndicator::modelChanged()
{
    m_pixmap.fill(Qt::transparent);

    if(m_model->visible()) {
        int r = skin()->ratio();
        QPainter paint(&m_pixmap);

        if(!m_model->elapsed())
        {
            paint.drawPixmap(r * 2, 0, skin()->getNumber(10));
        }

        int t = qAbs(m_model->displayTime());

        paint.drawPixmap(r * 13, 0, skin()->getNumber(t / 600 % 10));
        paint.drawPixmap(r * 26, 0, skin()->getNumber(t / 60 % 10));
        paint.drawPixmap(r * 43, 0, skin()->getNumber(t % 60 / 10));
        paint.drawPixmap(r * 56, 0, skin()->getNumber(t % 60 % 10));
    }

    setPixmap (m_pixmap);
}

void SkinnedTimeIndicator::mousePressEvent(QMouseEvent* e)
{
    if(m_model->visible() && e->button() & Qt::LeftButton)
    {
        m_model->toggleElapsed();
    }
    PixmapWidget::mousePressEvent(e);
}

void SkinnedTimeIndicator::updateSkin()
{
    m_pixmap = QPixmap(65 * skin()->ratio(),13 * skin()->ratio());
    modelChanged();
}

void SkinnedTimeIndicator::mouseMoveEvent(QMouseEvent *)
{}

void SkinnedTimeIndicator::mouseReleaseEvent(QMouseEvent *)
{}
