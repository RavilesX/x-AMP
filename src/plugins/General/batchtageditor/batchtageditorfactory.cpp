/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
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
#include "batchtageditor.h"
#include "batchtageditorfactory.h"

GeneralProperties BatchTagEditorFactory::properties() const
{
    GeneralProperties properties;
    properties.name = tr("Tag Editor Plugin");
    properties.shortName = "batchtageditor"_L1;
    properties.hasAbout = true;
    properties.hasSettings = false;
    properties.visibilityControl = false;
    return properties;
}

QObject *BatchTagEditorFactory::create(QObject *parent)
{
    return new BatchTagEditor(parent);
}

QDialog *BatchTagEditorFactory::createSettings(QWidget *)
{
    return nullptr;
}

void BatchTagEditorFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About Tag Editor Plugin"),
                       tr("x-AMP Tag Editor Plugin") + QChar::LineFeed +
                       tr("This plugin allows for batch editing of tags") + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QString BatchTagEditorFactory::translation() const
{
    return QLatin1String(":/batchtageditor_plugin_");
}
