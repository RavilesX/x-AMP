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
#include <QPainter>
#include <QResizeEvent>
#include <QMenu>
#include <QSettings>
#include <QApplication>
#include <qmmpui/playlistmodel.h>
#include "dock.h"
#include "windowsystem.h"
#include "skinnedbutton.h"
#include "skinnedplaylisttitlebar.h"
#include "skin.h"

#define TITLE_FORMAT u"%p%if(%p&%t, - ,)%t%if(%p,,%if(%t,,%f))%if(%l, %(%l%),)"_s

// TODO {shademode, updateskin} -> do we have the shaded cursor
SkinnedPlayListTitleBar::SkinnedPlayListTitleBar(QWidget *parent)
        : PixmapWidget(parent)
{
    m_formatter.setPattern(TITLE_FORMAT);
    m_ratio = skin()->ratio();
    m_pl = qobject_cast<SkinnedPlayList*>(parent);
    m_mw = qobject_cast<SkinnedMainWindow*>(m_pl->parent());

    m_close = new SkinnedButton(this,Skin::PL_BT_CLOSE_N, Skin::PL_BT_CLOSE_P, Skin::CUR_PCLOSE);
    connect(m_close, &SkinnedButton::clicked, m_pl, &SkinnedPlayList::closed);

    m_shade = new SkinnedButton(this, Skin::PL_BT_SHADE1_N, Skin::PL_BT_SHADE1_P, Skin::CUR_PWINBUT);
    connect(m_shade, &SkinnedButton::clicked, this, &SkinnedPlayListTitleBar::shade);

    resize(275 * m_ratio, 20 * m_ratio);
    setMinimumWidth(275 * m_ratio);

    readSettings();
    QSettings settings;
#ifdef QMMP_WS_X11
    if(m_pl->useCompiz())
        m_pl->setFixedSize(settings.value("Skinned/pl_size"_L1, QSize(m_ratio * 275, m_ratio * 116)).toSize());
    else
#endif
        m_pl->resize (settings.value ("Skinned/pl_size"_L1, QSize(m_ratio * 275, m_ratio * 116)).toSize());
    if(settings.value ("Skinned/pl_shaded"_L1, false).toBool())
        shade();
    resize(m_pl->width(),height());
    m_align = true;
    setCursor(skin()->getCursor(Skin::CUR_PTBAR));
    updatePositions();
}


SkinnedPlayListTitleBar::~SkinnedPlayListTitleBar()
{
    QSettings settings;
    settings.setValue("Skinned/pl_size"_L1, QSize (m_pl->width(), m_shaded ? m_height:m_pl->height()));
    settings.setValue("Skinned/pl_shaded"_L1, m_shaded);
}

void SkinnedPlayListTitleBar::updatePositions()
{
    m_ratio = skin()->ratio();
    int sx = (width() - 275 * m_ratio) / 25;
    m_close->move(m_ratio * 264 + sx * 25, m_ratio * 3);
    m_shade->move(m_ratio * 255 + sx * 25, m_ratio * 3);
    if(m_shade2)
        m_shade2->move(m_ratio * 255 + sx * 25, m_ratio * 3);
}

void SkinnedPlayListTitleBar::updatePixmap()
{
    int sx = ((m_shaded ? m_pl->width() : width()) - 275 * m_ratio) / 25;
    QPixmap pixmap(275 * m_ratio + sx * 25, 20 * m_ratio);
    QPainter paint;
    paint.begin(&pixmap);
    if(m_shaded)
    {
        paint.drawPixmap(0, 0, skin()->getPlPart(Skin::PL_TITLEBAR_SHADED2));
        for (int i = 1; i < sx + 9 * m_ratio; i++)
        {
            paint.drawPixmap(25 * i, 0, skin()->getPlPart(Skin::PL_TFILL_SHADED));
        }
    }

    if(m_active)
    {
        if(m_shaded)
            paint.drawPixmap(225 * m_ratio +sx * 25, 0, skin()->getPlPart(Skin::PL_TITLEBAR_SHADED1_A));
        else
        {
            paint.drawPixmap(0, 0, skin()->getPlPart(Skin::PL_CORNER_UL_A));
            for (int i = 1; i < sx + 10 * m_ratio; i++)
            {
                paint.drawPixmap(25 * i, 0, skin()->getPlPart(Skin::PL_TFILL1_A));
            }
            paint.drawPixmap((100 - 12) * m_ratio + 12 * sx, 0, skin()->getPlPart(Skin::PL_TITLEBAR_A));
            paint.drawPixmap(250 * m_ratio + sx * 25, 0, skin()->getPlPart(Skin::PL_CORNER_UR_A));
        }
    }
    else
    {
        if(m_shaded)
            paint.drawPixmap(225 * m_ratio + sx * 25, 0, skin()->getPlPart(Skin::PL_TITLEBAR_SHADED1_I));
        else
        {
            paint.drawPixmap(0, 0, skin()->getPlPart(Skin::PL_CORNER_UL_I));
            for (int i = 1; i < sx + 10 * m_ratio; i++)
            {
                paint.drawPixmap(25 * i, 0, skin()->getPlPart(Skin::PL_TFILL1_I));
            }
            paint.drawPixmap((100 - 12) * m_ratio + 12 * sx, 0, skin()->getPlPart(Skin::PL_TITLEBAR_I));
            paint.drawPixmap(250 * m_ratio + sx * 25, 0, skin()->getPlPart(Skin::PL_CORNER_UR_I));
        }
    }
    if(m_shaded)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        QColor col = QColor::fromString(QString(skin()->getPLValue("normalbg")));
#else
        QColor col;
        col.setNamedColor(QString(skin()->getPLValue("normalbg")));
#endif
        paint.setBrush(QBrush(col));
        paint.setPen(col);
        paint.drawRect(8 * m_ratio, m_ratio, 235 * m_ratio + sx * 25, 11 * m_ratio);
        //draw text
        paint.setFont(m_font);
        paint.setPen(QString(skin()->getPLValue("normal")));
        paint.drawText(9 * m_ratio, 11 * m_ratio, m_truncatedText);
    }
    paint.end();
    setPixmap(pixmap);
}

void SkinnedPlayListTitleBar::resizeEvent(QResizeEvent *)
{
    QFontMetrics metrics(m_font);
    m_truncatedText = metrics.elidedText(m_text, Qt::ElideRight, width() - 35 * m_ratio);
    updatePositions();
    updatePixmap();
}

void SkinnedPlayListTitleBar::mousePressEvent(QMouseEvent* event)
{
    switch (event->button ())
    {
    case Qt::LeftButton:
        pos = event->pos();
        if(m_shaded && (width() - 30 * m_ratio) < pos.x() && pos.x() < (width() - 22 * m_ratio))
        {
            m_resize = true;
            setCursor (Qt::SizeHorCursor);
        }
        break;
    case Qt::RightButton:
        m_mw->menu()->exec(event->globalPosition().toPoint());
        break;
    default:
        ;
    }
}

void SkinnedPlayListTitleBar::mouseReleaseEvent(QMouseEvent*)
{
    Dock::instance()->updateDock();
    m_resize = false;
    setCursor(skin()->getCursor(Skin::CUR_PTBAR));
}

void SkinnedPlayListTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    QPoint npos = event->globalPosition().toPoint() - pos;
    if(m_shaded && m_resize)
    {
#ifdef QMMP_WS_X11
        //avoid right corner moving during resize
        if(layoutDirection() == Qt::RightToLeft)
            WindowSystem::revertGravity(m_pl->winId());
#endif

        int dx = 25 * m_ratio;
        int sx = ((event->position().x() - 275 * m_ratio) + 14) / dx;
        sx = qMax(sx, 0);
        resize(275 * m_ratio + dx * sx, height());

#ifdef QMMP_WS_X11
        if(m_pl->useCompiz())

            m_pl->setFixedSize(275 * m_ratio + dx * sx, m_pl->height());
        else
#endif
            m_pl->resize(275 * m_ratio + dx * sx, m_pl->height());
    }
    else if(pos.x() < width() - 30*m_ratio)
        Dock::instance()->move(m_pl, npos);
}

void SkinnedPlayListTitleBar::setActive(bool a)
{
    m_active = a;
    updatePixmap();
}

void SkinnedPlayListTitleBar::setModel(PlayListModel *selected, PlayListModel *previous)
{
    if(previous)
        disconnect(previous, nullptr, this, nullptr); //disconnect previous model
    m_model = selected;
    connect(m_model, &PlayListModel::listChanged, this, &SkinnedPlayListTitleBar::showCurrent);
    showCurrent();
}

void SkinnedPlayListTitleBar::readSettings()
{
    QSettings settings;
    m_font.fromString(settings.value("Skinned/pl_font"_L1, QApplication::font().toString()).toString());
    m_font.setPixelSize(12 * m_ratio);
}

void SkinnedPlayListTitleBar::updateSkin()
{
    setCursor(skin()->getCursor(Skin::CUR_PTBAR));
    if(m_ratio != skin()->ratio())
    {
        m_ratio = skin()->ratio();
        m_font.setPixelSize(12 * m_ratio);
        setMinimumWidth(275 * m_ratio);
        updatePositions();
    }
    updatePixmap();
}

void SkinnedPlayListTitleBar::shade()
{
    m_shaded = !m_shaded;
    if(m_shaded)
    {
        m_height = m_pl->height();
        m_shade->hide();
        m_shade2 = new SkinnedButton(this, Skin::PL_BT_SHADE2_N, Skin::PL_BT_SHADE2_P, Skin::CUR_PWSNORM);
        m_shade2->move(254, 3);
        connect(m_shade2, &SkinnedButton::clicked, this, &SkinnedPlayListTitleBar::shade);
        m_shade2->show();
    }
    else
    {
        m_shade2->deleteLater();
        m_shade2 = nullptr;
        m_shade->show();
    }
    m_pl->setMinimalMode(m_shaded);
    showCurrent();
    update();
    if(m_align)
        Dock::instance()->align(m_pl, m_shaded? -m_height + 14 * m_ratio: m_height - 14 * m_ratio);
    updatePositions();
}

void SkinnedPlayListTitleBar::mouseDoubleClickEvent (QMouseEvent *)
{
    SkinnedPlayListTitleBar::shade();
}

void SkinnedPlayListTitleBar::showCurrent()
{
    m_text.clear();

    if(m_model)
    {
        PlayListTrack* track = m_model->currentTrack();
        if(track)
            m_text = QStringLiteral("%1. %2").arg(track->trackIndex() + 1).arg(m_formatter.format(track));
    }
    QFontMetrics metrics(m_font);
    m_truncatedText = metrics.elidedText(m_text, Qt::ElideRight, width() -  35 * m_ratio);
    updatePixmap();
}
