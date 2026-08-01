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
#include <QPalette>
#include <QSettings>
#include <QDir>
#include <QFont>
#include <qmmp/soundcore.h>
#include <qmmp/metadatamanager.h>
#include "popupwidget.h"

PopupWidget::PopupWidget(QWidget *parent)
        : QFrame(parent)
{
    setWindowFlags(Qt::X11BypassWindowManagerHint |
                   Qt::WindowStaysOnTopHint | Qt::Dialog | Qt::FramelessWindowHint);
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setAttribute(Qt::WA_QuitOnClose, false);
    QHBoxLayout *hlayout = new QHBoxLayout(this); //layout
    m_pixlabel = new QLabel(this);
    m_pixlabel->setPixmap(QPixmap(u":/notifier_icon.png"_s));
    m_pixlabel->setFixedSize(32,32);
    hlayout->addWidget(m_pixlabel);

    m_label1 = new QLabel(this);
    hlayout->addWidget (m_label1);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    //settings
    QSettings settings;
    settings.beginGroup(u"Notifier"_s);
    int delay = settings.value(u"message_delay"_s, 2000).toInt();
    m_pos = settings.value(u"message_pos"_s, PopupWidget::BOTTOMLEFT).toUInt();
    setWindowOpacity(settings.value(u"opacity"_s, 1.0).toDouble());
    QString fontname = settings.value(u"font"_s).toString();
    m_coverSize = settings.value(u"cover_size"_s, 64).toInt();
    m_formatter.setPattern(settings.value(u"template"_s, DEFAULT_TEMPLATE).toString());
    settings.endGroup();
    //font
    QFont font;
    if(!fontname.isEmpty())
        font.fromString(fontname);
    setFont(font);
    //timer
    m_timer = new QTimer(this);
    m_timer->setInterval(delay);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &PopupWidget::hide);
}

PopupWidget::~PopupWidget()
{}

void PopupWidget::mousePressEvent (QMouseEvent *)
{
    hide();
}

void PopupWidget::showMetaData()
{
    m_timer->stop();
    SoundCore *core = SoundCore::instance();

    QString title = m_formatter.format(core->trackInfo());

    m_label1->setText(title);

    QImage img = MetaDataManager::instance()->getCover(core->path());
    if(!img.isNull())
    {
        m_pixlabel->setFixedSize(m_coverSize,m_coverSize);
        m_pixlabel->setPixmap(QPixmap::fromImage(img.scaled(m_coverSize,m_coverSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    }
    else
    {
        m_pixlabel->setPixmap(QPixmap(u":/notifier_icon.png"_s));
        m_pixlabel->setFixedSize(32,32);
    }
    updateGeometry();
    setFixedSize(sizeHint());
    update();
    show();
    updatePosition();
    m_timer->start();
}

void PopupWidget::showVolume(int v)
{
    m_pixlabel->setPixmap(QPixmap(u":/notifier_icon.png"_s));
    m_pixlabel->setFixedSize(32,32);
    m_timer->stop();
    m_label1->setAlignment(Qt::AlignVCenter);
    m_label1->setText(u"<b>"_s + tr("Volume:") + QStringLiteral(" %1\%</b>").arg(v));

    updateGeometry();
    setFixedSize(sizeHint());
    update();
    show();
    updatePosition();
    m_timer->start();
}

void PopupWidget::updatePosition()
{
    //calculate widget position
    QRect desktopRect = QGuiApplication::primaryScreen()->availableGeometry();
    int x = desktopRect.x() + 5, y = desktopRect.y() + 5;
    if (m_pos == LEFT || m_pos == RIGHT || m_pos == CENTER)
        y =  desktopRect.y() + desktopRect.height()/2 - height()/2 + 5;
    else if (m_pos == BOTTOMLEFT || m_pos == BOTTOM || m_pos == BOTTOMRIGHT)
        y = desktopRect.y() + desktopRect.height() - height() - 5;
    if (m_pos == TOP || m_pos == BOTTOM || m_pos == CENTER)
        x = desktopRect.x() + desktopRect.width()/2 - width()/2;
    else if (m_pos == TOPRIGHT || m_pos == RIGHT || m_pos == BOTTOMRIGHT)
        x = desktopRect.x() + desktopRect.width() - width() - 5;
    move (x,y);
}
