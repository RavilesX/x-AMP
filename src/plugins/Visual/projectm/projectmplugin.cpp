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
#include <QTimer>
#include <QSettings>
#include <QPainter>
#include <QMenu>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QSplitter>
#include <QListWidget>
#include <stdlib.h>
#include <locale.h>
#include <qmmp/qmmp.h>
#ifdef PROJECTM_4
#include "projectm4widget.h"
#else
#include "projectmwidget.h"
#endif
#include "projectmplugin.h"

ProjectMPlugin::ProjectMPlugin (QWidget *parent)
        : Visual (parent, Qt::Window | Qt::MSWindowsOwnDC)
{
    setlocale(LC_NUMERIC, "C"); //fixes problem with non-english locales
    setWindowTitle(tr("ProjectM"));
    setWindowIcon(parent->windowIcon());

    m_splitter = new QSplitter(Qt::Horizontal, this);
    QListWidget *listWidget = new QListWidget(m_splitter);
    listWidget->setAlternatingRowColors(true);
    m_splitter->addWidget(listWidget);
#ifdef PROJECTM_4
    m_projectMWidget = new ProjectM4Widget(listWidget, m_splitter);
#else
    m_projectMWidget = new ProjectMWidget(listWidget, m_splitter);
#endif
    m_splitter->addWidget(m_projectMWidget);

    m_splitter->setStretchFactor(1,1);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_splitter);
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);
    addActions(m_projectMWidget->actions());
#ifdef PROJECTM_4
    connect(m_projectMWidget, &ProjectM4Widget::showMenuToggled, listWidget, &QListWidget::setVisible);
    connect(m_projectMWidget, &ProjectM4Widget::fullscreenToggled, this, &ProjectMPlugin::setFullScreen);
#else
    connect(m_projectMWidget, &ProjectMWidget::showMenuToggled, listWidget, &QListWidget::setVisible);
    connect(m_projectMWidget, &ProjectMWidget::fullscreenToggled, this, &ProjectMPlugin::setFullScreen);
#endif
    listWidget->hide();
    resize(600,400);
    QSettings settings;
    restoreGeometry(settings.value("ProjectM/geometry"_L1).toByteArray());
    m_splitter->setSizes(QList<int>() << 300 << 300);
    m_splitter->restoreState(settings.value("ProjectM/splitter_sizes"_L1).toByteArray());

    m_timer = new QTimer(this);
    m_timer->setInterval(0);
    connect(m_timer, &QTimer::timeout, this, &ProjectMPlugin::onTimeout);
}

ProjectMPlugin::~ProjectMPlugin()
{}

void ProjectMPlugin::start()
{
    if(isVisible())
        m_timer->start();
}

void ProjectMPlugin::stop()
{
    update();
}

void ProjectMPlugin::onTimeout()
{
    if(takeData(m_left, m_right))
    {
        m_projectMWidget->addPCM(m_left, m_right);
    }

    m_projectMWidget->update();
}

void ProjectMPlugin::setFullScreen(bool yes)
{
    if(yes)
        setWindowState(windowState() | Qt::WindowFullScreen);
    else
        setWindowState(windowState() & ~Qt::WindowFullScreen);
}

void ProjectMPlugin::closeEvent(QCloseEvent *event)
{
    //save geometry
    QSettings settings;
    settings.setValue("ProjectM/geometry"_L1, saveGeometry());
    settings.setValue("ProjectM/splitter_sizes"_L1, m_splitter->saveState());
    Visual::closeEvent(event); //removes visualization object
}

void ProjectMPlugin::showEvent(QShowEvent *)
{
    m_timer->start();
}

void ProjectMPlugin::hideEvent(QHideEvent *)
{
    m_timer->stop();
}
