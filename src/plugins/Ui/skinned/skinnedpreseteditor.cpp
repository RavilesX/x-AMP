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

#include <QIcon>
#include <qmmp/qmmp.h>
#include "ui_skinnedpreseteditor.h"
#include "skinnedpreseteditor.h"

SkinnedPresetEditor::SkinnedPresetEditor(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::SkinnedPresetEditor)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    connect(m_ui->loadButton, &QPushButton::clicked, this, &SkinnedPresetEditor::loadPreset);
    connect(m_ui->deleteButton, &QPushButton::clicked, this, &SkinnedPresetEditor::removePreset);
    m_ui->loadButton->setIcon(QIcon::fromTheme(u"document-open"_s));
    m_ui->deleteButton->setIcon(QIcon::fromTheme(u"edit-delete"_s));
}

SkinnedPresetEditor::~SkinnedPresetEditor()
{
    delete m_ui;
}

void SkinnedPresetEditor::addPresets(const QStringList &names)
{
    for(const QString &name : std::as_const(names))
    {
        m_ui->presetListWidget->addItem(name);
    }
}

void SkinnedPresetEditor::addAutoPresets(const QStringList &names)
{
    for(const QString &name : std::as_const(names))
    {
        m_ui->autoPresetListWidget->addItem(name);
    }
}

void SkinnedPresetEditor::loadPreset()
{
    QListWidgetItem *item = nullptr;
    if(m_ui->tabWidget->currentIndex() == 0)
        item = m_ui->presetListWidget->currentItem();
    if(m_ui->tabWidget->currentIndex () == 1)
        item = m_ui->autoPresetListWidget->currentItem();
    if(item)
        emit presetLoaded(item->text(), m_ui->tabWidget->currentIndex () == 1);
}

void SkinnedPresetEditor::removePreset()
{
    QListWidgetItem *item = nullptr;
    if(m_ui->tabWidget->currentIndex() == 0)
        item = m_ui->presetListWidget->currentItem();
    if(m_ui->tabWidget->currentIndex () == 1)
        item = m_ui->autoPresetListWidget->currentItem();
    if(item)
    {
        emit presetRemoved(item->text(), m_ui->tabWidget->currentIndex () == 1);
        delete item;
    }
}
