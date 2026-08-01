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
#include <QPixmap>
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QSettings>
#include <QDir>
#include <qmmp/qmmp.h>
#include <qmmpui/filedialog.h>
#include "coverwidget.h"

CoverWidget::CoverWidget(QWidget *parent)
        : QWidget(parent)
{
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose, true);
    m_menu = new QMenu(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
    m_menu->addAction(tr("&Save As..."), this, &CoverWidget::saveAs, tr("Ctrl+S"));
#else
    m_menu->addAction(tr("&Save As..."), tr("Ctrl+S"), this, &CoverWidget::saveAs);
#endif
    QMenu *sizeMenu = m_menu->addMenu(tr("Size"));
    QActionGroup *sizeGroup = new QActionGroup(this);
    sizeGroup->addAction(tr("Actual Size"))->setData(0);
    sizeGroup->addAction(tr("128x128"))->setData(128);
    sizeGroup->addAction(tr("256x256"))->setData(256);
    sizeGroup->addAction(tr("512x512"))->setData(512);
    sizeGroup->addAction(tr("1024x1024"))->setData(1024);
    sizeMenu->addActions(sizeGroup->actions());
    connect(sizeMenu, &QMenu::triggered, this, &CoverWidget::processResizeAction);
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
    m_menu->addAction(tr("&Close"), this, &CoverWidget::close, tr("Alt+F4"));
#else
    m_menu->addAction(tr("&Close"), tr("Alt+F4"), this, &CoverWidget::close);
#endif
    addActions(m_menu->actions());
    m_size = 0;
    //settings
    QSettings settings;
    m_size = settings.value(u"CoverManager/size"_s, 0).toInt();
    for(QAction *a : sizeMenu->actions())
    {
        a->setCheckable(true);
        if(a->data().toInt() == m_size)
        {
            a->setChecked(true);
            processResizeAction(a);
        }
    }
}

void CoverWidget::setImage(const QImage &img)
{
    m_image = img;
    if(m_size == 0)
       resize(m_image.size());
    update();
}

void CoverWidget::paintEvent(QPaintEvent *p)
{
    QPainter paint(this);
    if(!m_image.isNull())
        paint.drawImage(0 ,0, m_image.scaled(p->rect().size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

void CoverWidget::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::RightButton)
        m_menu->exec(e->globalPosition().toPoint());
}

void CoverWidget::saveAs()
{
    QString path = FileDialog::getSaveFileName(this, tr("Save Cover As"), QDir::homePath() + u"/cover.jpg"_s,
                                               tr("Images") + u" (*.png *.jpg)"_s);

    if (!path.isEmpty())
        m_image.save(path);
}

void CoverWidget::processResizeAction(QAction *action)
{
    m_size = action->data().toInt();
    if(m_size == 0)
        resize(m_image.size());
    else
        resize(m_size, m_size);
    update();
    QSettings settings;
    settings.setValue(u"CoverManager/size"_s, m_size);
}
