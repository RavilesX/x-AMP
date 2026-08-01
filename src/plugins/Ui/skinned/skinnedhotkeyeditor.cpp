/***************************************************************************
 *   Copyright (C) 2011-2026 by Ilya Kotov                                 *
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
#include <QAction>
#include <qmmpui/shortcutdialog.h>
#include "skinnedactionmanager.h"
#include "skinnedhotkeyeditor.h"
#include "skinnedshortcutitem.h"
#include "ui_skinnedhotkeyeditor.h"

SkinnedHotkeyEditor::SkinnedHotkeyEditor(QWidget *parent) : QWidget(parent), m_ui(new Ui::SkinnedHotkeyEditor)
{
    m_ui->setupUi(this);
    loadShortcuts();
}

SkinnedHotkeyEditor::~SkinnedHotkeyEditor()
{
    delete m_ui;
}

void SkinnedHotkeyEditor::applyShortcuts()
{
    for(int i = 0; i < m_ui->shortcutTreeWidget->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = m_ui->shortcutTreeWidget->topLevelItem(i);
        for(int j = 0; j < item->childCount(); ++j)
        {
            static_cast<SkinnedShortcutItem *>(item->child(j))->applyShortcut();
        }
    }
}

void SkinnedHotkeyEditor::on_changeShortcutButton_clicked()
{
    SkinnedShortcutItem *item = dynamic_cast<SkinnedShortcutItem *>(m_ui->shortcutTreeWidget->currentItem());
    if(!item)
        return;
    ShortcutDialog editor(item->shortcut(), this);
    if(editor.exec() == QDialog::Accepted)
    {
        item->setShortcut(editor.key());
        item->setText(1, item->shortcut().toString(QKeySequence::NativeText));
    }
}

void SkinnedHotkeyEditor::loadShortcuts()
{
    m_ui->shortcutTreeWidget->clear();
    //playback
    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Playback") });
    for(int i = SkinnedActionManager::PLAY; i <= SkinnedActionManager::CLEAR_QUEUE; ++i)
        new SkinnedShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //view
    item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("View") });
    for(int i = SkinnedActionManager::SHOW_PLAYLIST; i <= SkinnedActionManager::WM_DOUBLE_SIZE; ++i)
        new SkinnedShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //volume
    item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Volume") });
    for(int i = SkinnedActionManager::VOL_ENC; i <= SkinnedActionManager::VOL_MUTE; ++i)
        new SkinnedShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //playlist
    item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Playlist") });
    for(int i = SkinnedActionManager::PL_ADD_FILE; i <= SkinnedActionManager::PL_SHOW_TABBAR; ++i)
        new SkinnedShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //misc
    item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Misc") });
    for(int i = SkinnedActionManager::SETTINGS; i <= SkinnedActionManager::QUIT; ++i)
        new SkinnedShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);

    m_ui->shortcutTreeWidget->resizeColumnToContents(0);
    m_ui->shortcutTreeWidget->resizeColumnToContents(1);
}

void SkinnedHotkeyEditor::on_restoreShortcutsButton_clicked()
{
    if(QMessageBox::question(this, tr("Reset Shortcuts"),
                             tr("Do you want to restore default shortcuts?"),
                             QMessageBox::Yes | QMessageBox::No) ==  QMessageBox::Yes)
    {
        SkinnedActionManager::instance()->resetShortcuts();
        loadShortcuts();
    }
}
