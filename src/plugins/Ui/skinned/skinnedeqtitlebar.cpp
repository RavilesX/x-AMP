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
#include <QMouseEvent>
#include <QMenu>
#include <QSettings>
#include <qmmp/soundcore.h>
#include "skin.h"
#include "shadedbar.h"
#include "dock.h"
#include "skinnedmainwindow.h"
#include "skinnedbutton.h"
#include "skinnedeqwidget.h"
#include "skinnedeqtitlebar.h"

SkinnedEqTitleBar::SkinnedEqTitleBar(SkinnedEqWidget *parent)
        : PixmapWidget(parent)
{
    m_eq = parent;
    m_mw = qobject_cast<SkinnedMainWindow*>(m_eq->parent());
    m_close = new SkinnedButton(this, Skin::EQ_BT_CLOSE_N, Skin::EQ_BT_CLOSE_P, Skin::CUR_EQCLOSE);
    connect(m_close, &SkinnedButton::clicked, m_eq, &SkinnedEqWidget::closed);
    m_shade = new SkinnedButton(this, Skin::EQ_BT_SHADE1_N, Skin::EQ_BT_SHADE1_P, Skin::CUR_EQNORMAL);
    connect(m_shade, &SkinnedButton::clicked, this, &SkinnedEqTitleBar::shade);
    QSettings settings;
    if(settings.value("Skinned/eq_shaded"_L1, false).toBool())
        shade();
    m_align = true;
    setActive(false);
    setCursor(skin()->getCursor(Skin::CUR_EQTITLE));
    updatePositions();
}


SkinnedEqTitleBar::~SkinnedEqTitleBar()
{
    QSettings settings;
    settings.setValue("Skinned/eq_shaded"_L1, m_shaded);
}

void SkinnedEqTitleBar::updatePositions()
{
     m_close->move(skin()->scaled(264), skin()->scaled(3));
     m_shade->move(skin()->scaled(254), skin()->scaled(3));
     if(m_volumeBar)
         m_volumeBar->move(skin()->scaled(61), skin()->scaled(4));
     if(m_balanceBar)
         m_balanceBar->move(skin()->scaled(164), skin()->scaled(4));
     if(m_shade2)
         m_shade2->move(skin()->scaled(254), skin()->scaled(3));
}

void SkinnedEqTitleBar::setActive(bool active)
{
    if(active)
    {
        if(m_shaded)
            setPixmap(skin()->getEqPart(Skin::EQ_TITLEBAR_SHADED_A));
        else
            setPixmap(skin()->getEqPart(Skin::EQ_TITLEBAR_A));
    }
    else
    {
        if(m_shaded)
            setPixmap(skin()->getEqPart(Skin::EQ_TITLEBAR_SHADED_I));
        else
            setPixmap(skin()->getEqPart(Skin::EQ_TITLEBAR_I));
    }
}

void SkinnedEqTitleBar::mousePressEvent(QMouseEvent *event)
{
    switch(event->button ())
    {
    case Qt::LeftButton:
        m_pos = event->pos();
        break;
    case Qt::RightButton:
        m_mw->menu()->exec(event->globalPosition().toPoint());
        break;
    default:
        ;
    }
}

void SkinnedEqTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if(m_pos.x() < width() - skin()->scaled(30))
    {
        QPoint npos = (event->globalPosition().toPoint() - m_pos);
        Dock::instance()->move(m_eq, npos);
    }
}

void SkinnedEqTitleBar::mouseReleaseEvent(QMouseEvent*)
{
    Dock::instance()->updateDock();
}

void SkinnedEqTitleBar::mouseDoubleClickEvent(QMouseEvent *)
{
    SkinnedEqTitleBar::shade();
}

void SkinnedEqTitleBar::shade()
{
    m_shaded = !m_shaded;

    if(m_shaded)
    {
        setPixmap(skin()->getEqPart(Skin::EQ_TITLEBAR_SHADED_A));
        m_shade->hide();
        m_shade2 = new SkinnedButton(this, Skin::EQ_BT_SHADE2_N, Skin::EQ_BT_SHADE2_P, Skin::CUR_EQNORMAL);
        m_shade2->move(skin()->scaled(254), skin()->scaled(3));
        connect(m_shade2, &SkinnedButton::clicked, this, &SkinnedEqTitleBar::shade);
        m_shade2->show();
        m_volumeBar = new ShadedBar(this, Skin::EQ_VOLUME1, Skin::EQ_VOLUME2, Skin::EQ_VOLUME3);
        m_volumeBar->move(skin()->scaled(61), skin()->scaled(4));
        m_volumeBar->show();
        m_balanceBar = new ShadedBar(this, Skin::EQ_BALANCE1, Skin::EQ_BALANCE2, Skin::EQ_BALANCE3);
        m_balanceBar->move(skin()->scaled(164), skin()->scaled(4));
        m_balanceBar->setRange(-100, 100);
        m_balanceBar->show();
        SoundCore *core = SoundCore::instance();
        connect(core, &SoundCore::volumeChanged, m_volumeBar, &ShadedBar::setValue);
        connect(core, &SoundCore::balanceChanged, m_balanceBar, &ShadedBar::setValue);
        connect(m_volumeBar, &ShadedBar::sliderMoved, core, &SoundCore::setVolume);
        connect(m_balanceBar, &ShadedBar::sliderMoved, core, &SoundCore::setBalance);
        m_volumeBar->setValue(core->volume());
        m_balanceBar->setValue(core->balance());
    }
    else
    {
        setPixmap(skin()->getEqPart(Skin::EQ_TITLEBAR_A));
        m_shade2->deleteLater();
        m_volumeBar->deleteLater();
        m_balanceBar->deleteLater();
        m_volumeBar = nullptr;
        m_balanceBar = nullptr;
        m_shade2 = nullptr;
        m_shade->show();
    }
    qobject_cast<SkinnedEqWidget *>(m_eq)->setMimimalMode(m_shaded);
    if(m_align)
        Dock::instance()->align(m_eq, m_shaded? -skin()->scaled(102): skin()->scaled(102));
}

void SkinnedEqTitleBar::updateSkin()
{
    setCursor(skin()->getCursor(Skin::CUR_EQTITLE));
    updatePositions();
}
