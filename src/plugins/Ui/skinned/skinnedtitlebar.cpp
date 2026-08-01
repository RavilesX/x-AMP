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

#include <QMainWindow>
#include <QApplication>
#include <QMouseEvent>
#include <QMenu>
#include <QSettings>
#include <qmmpui/mediaplayer.h>
#include "symboldisplay.h"
#include "skin.h"
#include "skinnedbutton.h"
#include "dock.h"
#include "skinnedtitlebarcontrol.h"
#include "shadedvisual.h"
#include "skinneddisplay.h"
#include "skinnedtitlebar.h"
#include "skinnedmainwindow.h"
#include "skinnedtimeindicator.h"

// TODO skin cursor with shade mode
SkinnedTitleBar::SkinnedTitleBar(SkinnedTimeIndicatorModel *model, QWidget *parent)
        : PixmapWidget(parent),
          m_model(model)
{
    setPixmap(skin()->getTitleBar(Skin::TITLEBAR_A));
    m_mw = qobject_cast<SkinnedMainWindow*>(parent->parent());
    //buttons
    m_menu = new SkinnedButton(this, Skin::BT_MENU_N, Skin::BT_MENU_P, Skin::CUR_MAINMENU);
    connect(m_menu, &SkinnedButton::clicked, this, &SkinnedTitleBar::showMainMenu);
    m_menu->move(6, 3);
    m_minimize = new SkinnedButton(this, Skin::BT_MINIMIZE_N, Skin::BT_MINIMIZE_P, Skin::CUR_MIN);
    connect(m_minimize, &SkinnedButton::clicked, m_mw, &SkinnedMainWindow::showMinimized);
    m_shade = new SkinnedButton(this, Skin::BT_SHADE1_N, Skin::BT_SHADE1_P, Skin::CUR_WINBUT);
    connect(m_shade, &SkinnedButton::clicked, this, &SkinnedTitleBar::shade);
    m_close = new SkinnedButton(this, Skin::BT_CLOSE_N,Skin::BT_CLOSE_P, Skin::CUR_CLOSE);
    connect(m_close, &SkinnedButton::clicked, m_mw, &SkinnedMainWindow::close);
    setActive(false);
    QSettings settings;
    if(settings.value("Skinned/disp_shaded"_L1, false).toBool())
        shade();
    m_align = true;
    setCursor(skin()->getCursor(Skin::CUR_TITLEBAR));
    updatePositions();
    connect(m_model, &SkinnedTimeIndicatorModel::changed, this, &SkinnedTitleBar::onModelChanged);
}

SkinnedTitleBar::~SkinnedTitleBar()
{
    QSettings settings;
    settings.setValue("Skinned/disp_shaded"_L1, m_shaded);
}

void SkinnedTitleBar::updatePositions()
{
    int r = skin()->ratio();
    m_menu->move(r * 6, r * 3);
    m_minimize->move(r * 244, r * 3);
    m_shade->move(r * 254, r*3);
    m_close->move(r * 264, r * 3);
    if(m_shade2)
        m_shade2->move(r * 254, r * 3);
    if(m_currentTime)
        m_currentTime->move(r * 127, r * 4);
    if(m_control)
        m_control->move(r * 168, r * 2);
    if(m_visual)
        m_visual->move(r * 79, r * 5);
}

void SkinnedTitleBar::mousePressEvent(QMouseEvent* event)
{
    switch ((int) event->button ())
    {
    case Qt::LeftButton:
        m_pos = event->pos();
        Dock::instance()->calculateDistances();
        Dock::instance()->updateDock();
        break;
    case Qt::RightButton:
        m_mw->menu()->exec(event->globalPosition().toPoint());
    }
}

void SkinnedTitleBar::mouseReleaseEvent(QMouseEvent*)
{
    Dock::instance()->updateDock();
}
void SkinnedTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if(m_pos.x() < width() - skin()->ratio() * 37)
    {
        QPoint npos = (event->globalPosition().toPoint() - m_pos);
        Dock::instance()->move(m_mw, npos);
    }
}

void SkinnedTitleBar::setActive(bool a)
{
    if(a)
    {
        if(m_shaded)
            setPixmap(skin()->getTitleBar(Skin::TITLEBAR_SHADED_A));
        else
            setPixmap(skin()->getTitleBar(Skin::TITLEBAR_A));
    }
    else
    {
        if(m_shaded)
            setPixmap(skin()->getTitleBar(Skin::TITLEBAR_SHADED_I));
        else
            setPixmap(skin()->getTitleBar(Skin::TITLEBAR_I));
    }
}

void SkinnedTitleBar::updateSkin()
{
    setActive(false);
    setCursor(skin()->getCursor(Skin::CUR_TITLEBAR));
    updatePositions();
}

void SkinnedTitleBar::showMainMenu()
{
    m_mw->menu()->exec(m_menu->mapToGlobal(m_menu->pos()));
}

void SkinnedTitleBar::shade()
{
    m_shaded = !m_shaded;
    int r = skin()->ratio();
    if(m_shaded)
    {
        setPixmap(skin()->getTitleBar(Skin::TITLEBAR_SHADED_A));
        m_shade->hide();
        m_shade2 = new SkinnedButton(this,Skin::BT_SHADE2_N, Skin::BT_SHADE2_P, Skin::CUR_WSNORMAL);
        connect(m_shade2, &SkinnedButton::clicked, this, &SkinnedTitleBar::shade);
        m_shade2->show();
        m_currentTime = new SymbolDisplay(this, 6);
        m_currentTime->show();
        connect(m_currentTime, &SymbolDisplay::mouseClicked, m_model, &SkinnedTimeIndicatorModel::toggleElapsed);
        m_control = new SkinnedTitleBarControl(this);
        m_control->show();
        MediaPlayer *player = MediaPlayer::instance();
        connect(m_control, &SkinnedTitleBarControl::nextClicked, player, &MediaPlayer::next);
        connect(m_control, &SkinnedTitleBarControl::previousClicked, player, &MediaPlayer::previous);
        connect(m_control, &SkinnedTitleBarControl::playClicked, player, &MediaPlayer::play);
        connect(m_control, &SkinnedTitleBarControl::pauseClicked, player, &MediaPlayer::pause);
        connect(m_control, &SkinnedTitleBarControl::stopClicked, player, &MediaPlayer::stop);
        connect(m_control, &SkinnedTitleBarControl::ejectClicked, m_mw, &SkinnedMainWindow::playFiles);
        m_visual = new ShadedVisual(this);
        Visual::add(m_visual);
        m_visual->show();
    }
    else
    {
        setPixmap(skin()->getTitleBar(Skin::TITLEBAR_A));
        m_shade2->deleteLater();
        m_currentTime->deleteLater();
        m_control->deleteLater();
        Visual::remove(m_visual);
        m_visual->deleteLater();
        m_shade2 = nullptr;
        m_currentTime = nullptr;
        m_control = nullptr;
        m_visual = nullptr;
        m_shade->show();
    }
    qobject_cast<SkinnedDisplay *> (parent())->setMinimalMode(m_shaded);
    if(m_align)
        Dock::instance()->align(m_mw, m_shaded ? -r * 102: r * 102);
    onModelChanged();
    updatePositions();
}

void SkinnedTitleBar::mouseDoubleClickEvent (QMouseEvent *)
{
    SkinnedTitleBar::shade();
}

QString SkinnedTitleBar::formatTime (int sec)
{
    bool sign = false;
    if(sec < 0) {
        sign = true;
        sec = -sec;
    }
    int minutes = sec / 60;
    int seconds = sec % 60;

    QString str_minutes = QString::number (minutes);
    QString str_seconds = QString::number (seconds);

    if(minutes < 10) str_minutes.prepend(QLatin1Char('0'));
    if(seconds < 10) str_seconds.prepend(QLatin1Char('0'));

    return (sign ? u"-"_s : QString()) + str_minutes + QLatin1Char(':') + str_seconds;
}

void SkinnedTitleBar::onModelChanged()
{
    if(!m_currentTime)
        return;

    if(!m_model->visible())
    {
        m_currentTime->display(u"  :  "_s);
    }
    else if(m_model->position() < 0)
    {
        m_currentTime->display(u"--:--"_s);
    }
    else
    {
        m_currentTime->display(formatTime(m_model->displayTime()));
    }
}
