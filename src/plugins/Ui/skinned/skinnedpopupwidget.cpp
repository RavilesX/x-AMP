/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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

#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QSettings>
#include <qmmp/soundcore.h>
#include <qmmp/metadatamanager.h>
#include <qmmpui/metadataformatter.h>
#include <qmmpui/playlistitem.h>
#include "skinnedpopupwidget.h"

SkinnedPopupWidget::SkinnedPopupWidget(QWidget *parent)
        : QWidget(parent)
{
    setWindowFlags(Qt::ToolTip | Qt::BypassGraphicsProxyWidget);
    //setFrameStyle(QFrame::Box | QFrame::Plain);
    setAttribute(Qt::WA_QuitOnClose, false);
    QHBoxLayout *hlayout = new QHBoxLayout(this); //layout
    m_pixlabel = new QLabel(this);
    hlayout->addWidget(m_pixlabel);

    m_label1 = new QLabel(this);
    hlayout->addWidget (m_label1);

    //settings
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    setWindowOpacity(settings.value("popup_opacity"_L1, 1.0).toDouble());
    m_coverSize = settings.value("popup_cover_size"_L1, 48).toInt();
    m_template = settings.value("popup_template"_L1, DEFAULT_TEMPLATE).toString();
    m_formatter.setPattern(m_template);
    int delay = settings.value("popup_delay"_L1, 2500).toInt();
    bool show_cover = settings.value("popup_show_cover"_L1, true).toBool();
    settings.endGroup();
    //timer
    m_timer = new QTimer(this);
    m_timer->setInterval(delay);
    m_timer->setSingleShot (true);
    connect(m_timer, &QTimer::timeout, this, &SkinnedPopupWidget::show);
    if(show_cover)
        connect(m_timer, &QTimer::timeout, this, &SkinnedPopupWidget::loadCover);
    else
        m_pixlabel->hide();
    setMouseTracking(true);
}

void SkinnedPopupWidget::mousePressEvent (QMouseEvent *)
{
    hide();
}

void SkinnedPopupWidget::mouseMoveEvent (QMouseEvent *)
{
    hide();
}

void SkinnedPopupWidget::prepare(PlayListTrack *item, QPoint pos)
{
    pos += QPoint(15, 10);
    hide();
    if(!item)
    {
        m_timer->stop();
        m_url.clear();
        return;
    }
    m_url = item->path();
    m_label1->setText(m_formatter.format(item));
    qApp->processEvents();
    updateGeometry ();
    resize(sizeHint());
    qApp->processEvents();
    m_timer->start();
    QRect rect = QGuiApplication::primaryScreen()->availableGeometry();
    if(pos.x() + width() > rect.x() + rect.width())
        pos.rx() -= width();
    move(pos);
}

void SkinnedPopupWidget::deactivate()
{
    m_timer->stop();
    m_url.clear();
    hide();
}

QString SkinnedPopupWidget::url() const
{
    return m_url;
}

void SkinnedPopupWidget::loadCover()
{
    if(m_url.isEmpty())
        return;
    QImage img = MetaDataManager::instance()->getCover(m_url);
    if(img.isNull())
        img = QImage(u":/skinned/ui_no_cover.png"_s);
    m_pixlabel->setFixedSize(m_coverSize,m_coverSize);
    m_pixlabel->setPixmap(QPixmap::fromImage(img.scaled(m_coverSize, m_coverSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    qApp->processEvents();
    updateGeometry ();
    resize(sizeHint());
    qApp->processEvents();
}
