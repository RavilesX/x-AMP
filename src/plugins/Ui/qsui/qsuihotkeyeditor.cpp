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
#include <qmmpui/shortcutdialog.h>
#include <qmmp/qmmp.h>
#include "qsuiactionmanager.h"
#include "qsuihotkeyeditor.h"
#include "qsuishortcutitem.h"
#include "ui_qsuihotkeyeditor.h"

QSUiHotkeyEditor::QSUiHotkeyEditor(QWidget *parent) : QWidget(parent), m_ui(new Ui::QSUiHotkeyEditor)
{
    m_ui->setupUi(this);
    loadShortcuts();
    m_ui->changeShortcutButton->setIcon(QIcon::fromTheme(u"configure"_s));
}

QSUiHotkeyEditor::~QSUiHotkeyEditor()
{
    delete m_ui;
}

void QSUiHotkeyEditor::applyShortcuts()
{
    for(int i = 0; i < m_ui->shortcutTreeWidget->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = m_ui->shortcutTreeWidget->topLevelItem(i);
        for(int j = 0; j < item->childCount(); ++j)
        {
            static_cast<QSUiShortcutItem *>(item->child(j))->applyShortcut();
        }
    }
}

void QSUiHotkeyEditor::on_changeShortcutButton_clicked()
{
    QSUiShortcutItem *item = dynamic_cast<QSUiShortcutItem *>(m_ui->shortcutTreeWidget->currentItem());
    if(!item)
        return;
    ShortcutDialog editor(item->shortcut(), this);
    if(editor.exec() == QDialog::Accepted)
    {
        item->setShortcut(editor.key());
        item->setText(1, item->shortcut().toString(QKeySequence::NativeText));
    }
}

void QSUiHotkeyEditor::on_restoreShortcutsButton_clicked()
{
    if(QMessageBox::question(this, tr("Reset Shortcuts"),
                             tr("Do you want to restore default shortcuts?"),
                             QMessageBox::Yes | QMessageBox::No) ==  QMessageBox::Yes)
    {
        QSUiActionManager::instance()->resetShortcuts();
        loadShortcuts();
    }
}

void QSUiHotkeyEditor::loadShortcuts()
{
    m_ui->shortcutTreeWidget->clear();
    //playback
    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Playback") });
    for(int i = QSUiActionManager::PLAY; i <= QSUiActionManager::CLEAR_QUEUE; ++i)
        new QSUiShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //view
    item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("View") });
    for(int i = QSUiActionManager::WM_ALLWAYS_ON_TOP; i <= QSUiActionManager::UI_SHOW_STATUSBAR; ++i)
        new QSUiShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //volume
    item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Volume") });
    for(int i = QSUiActionManager::VOL_ENC; i <= QSUiActionManager::VOL_MUTE; ++i)
        new QSUiShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //playlist
    item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Playlist") });
    for(int i = QSUiActionManager::PL_ADD_FILE; i <= QSUiActionManager::PL_SHOW_HEADER; ++i)
        new QSUiShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //misc
    item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Misc") });
    for(int i = QSUiActionManager::EQUALIZER; i <= QSUiActionManager::QUIT; ++i)
        new QSUiShortcutItem(item, i);
    item->setExpanded(true);
    m_ui->shortcutTreeWidget->addTopLevelItem(item);
    //tools
    if(QSUiActionManager::instance()->hasDockWidgets())
    {
        item = new QTreeWidgetItem(m_ui->shortcutTreeWidget, { tr("Tools") });
        for(QDockWidget *w : QSUiActionManager::instance()->dockWidgtes())
            new QSUiShortcutItem(item, w);
        item->setExpanded(true);
        m_ui->shortcutTreeWidget->addTopLevelItem(item);
    }

    m_ui->shortcutTreeWidget->resizeColumnToContents(0);
    m_ui->shortcutTreeWidget->resizeColumnToContents(1);
}
