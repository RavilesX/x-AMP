/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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

#include <QAction>
#include <QDockWidget>
#include "qsuiactionmanager.h"
#include "qsuishortcutitem.h"

QSUiShortcutItem::QSUiShortcutItem(QTreeWidgetItem *parent, int type) :
    QTreeWidgetItem(parent, { QSUiActionManager::instance()->action(type)->text().remove(QLatin1Char('&')),
                    QSUiActionManager::instance()->action(type)->shortcut().toString(QKeySequence::NativeText) }),
    m_action(QSUiActionManager::instance()->action(type))
{
    m_shortcut = m_action->shortcut();
    setIcon(0, m_action->icon());
}

QSUiShortcutItem::QSUiShortcutItem(QTreeWidgetItem *parent, QDockWidget *w) :
    QTreeWidgetItem(parent, { w->toggleViewAction()->text().remove(QLatin1Char('&')),
                    w->toggleViewAction()->shortcut().toString(QKeySequence::NativeText) }),
    m_action(w->toggleViewAction())
{
    m_shortcut = m_action->shortcut();
}

QSUiShortcutItem::~QSUiShortcutItem()
{}

QKeySequence QSUiShortcutItem::shortcut() const
{
    return m_shortcut;
}

void QSUiShortcutItem::setShortcut(const QKeySequence &s)
{
    m_shortcut = s;
}

void QSUiShortcutItem::applyShortcut()
{
    m_action->setShortcut(m_shortcut);
}
