/***************************************************************************
 *   Copyright (C) 2018-2026 by Ilya Kotov                                 *
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

#include <QMessageBox>
#include "taskbarhelper.h"
#include "taskbarfactory.h"

GeneralProperties TaskbarFactory::properties() const
{
    GeneralProperties properties;
    properties.name = tr("Taskbar Plugin");
    properties.shortName = "taskbar"_L1;
    properties.hasAbout = true;
    properties.hasSettings = false;
    properties.visibilityControl = false;
    return properties;
}

QObject *TaskbarFactory::create(QObject *parent)
{
    return new TaskbarHelper(parent);
}

QDialog *TaskbarFactory::createSettings(QWidget *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

void TaskbarFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About Taskbar Plugin"),
                       tr("x-AMP Taskbar Plugin")+QChar::LineFeed+
                       tr("This plugin adds support for progress indicator in the Windows taskbar")+QChar::LineFeed+
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>") + QChar::LineFeed +
                       tr("Based on QtWinExtras module of the Qt Toolkit") + QChar::LineFeed +
                       tr("QtWinExtras developers:") + QChar::LineFeed +
                       tr("Ivan Vizir <define-true-false@yandex.com>") + QChar::LineFeed +
                       tr("The Qt Company Ltd."));
}

QString TaskbarFactory::translation() const
{
    return QLatin1String(":/taskbar_plugin_");
}
