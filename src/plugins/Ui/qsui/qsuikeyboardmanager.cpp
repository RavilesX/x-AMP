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

#include <QKeySequence>
#include <QAction>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/mediaplayer.h>
#include <qmmp/soundcore.h>
#include "qsuilistwidget.h"
#include "qsuikeyboardmanager.h"

QSUiKeyboardManager::QSUiKeyboardManager(QObject *parent) :
    QObject(parent)
{
    addAction(Qt::Key_Up, &QSUiKeyboardManager::processUp);
    addAction(Qt::Key_Up | Qt::ShiftModifier, &QSUiKeyboardManager::processUp);
    addAction(Qt::Key_Up | Qt::AltModifier, &QSUiKeyboardManager::processUp);
    addAction(Qt::Key_Up | Qt::ControlModifier, &QSUiKeyboardManager::processUp);

    addAction(Qt::Key_Down, &QSUiKeyboardManager::processDown);
    addAction(Qt::Key_Down | Qt::ShiftModifier, &QSUiKeyboardManager::processDown);
    addAction(Qt::Key_Down | Qt::AltModifier, &QSUiKeyboardManager::processDown);
    addAction(Qt::Key_Down | Qt::ControlModifier, &QSUiKeyboardManager::processDown);

    addAction(Qt::Key_Return, &QSUiKeyboardManager::processEnter);
    addAction(Qt::Key_PageUp, &QSUiKeyboardManager::processPgUp);
    addAction(Qt::Key_PageUp | Qt::ShiftModifier, &QSUiKeyboardManager::processPgUp);
    addAction(Qt::Key_PageDown, &QSUiKeyboardManager::processPgDown);
    addAction(Qt::Key_PageDown | Qt::ShiftModifier, &QSUiKeyboardManager::processPgDown);
    addAction(Qt::Key_Home, &QSUiKeyboardManager::processHome);
    addAction(Qt::Key_Home | Qt::ShiftModifier, &QSUiKeyboardManager::processHome);
    addAction(Qt::Key_End, &QSUiKeyboardManager::processEnd);
    addAction(Qt::Key_End | Qt::ShiftModifier, &QSUiKeyboardManager::processEnd);
}

QList<QAction *> QSUiKeyboardManager::actions()
{
    return m_actions;
}

void QSUiKeyboardManager::processUp()
{
    if(!m_listWidget || m_listWidget->filterMode())
        return;

    QKeyCombination keys = qobject_cast<QAction *>(sender())->shortcut()[0];

    QList<int> lines = m_listWidget->model()->selectedLines();

    if(lines.isEmpty())
    {
        m_listWidget->model()->setSelectedLine(m_listWidget->firstVisibleLine(), true);
        m_listWidget->setAnchorLine(m_listWidget->firstVisibleLine());
        return;
    }

    if (!(keys.keyboardModifiers() & Qt::ShiftModifier || keys.keyboardModifiers() & Qt::AltModifier ||
          keys.keyboardModifiers() & Qt::ControlModifier))
    {
        m_listWidget->model()->clearSelection();
        m_listWidget->setAnchorLine(-1);
    }

    int first_visible = m_listWidget->firstVisibleLine();
    int last_visible = m_listWidget->visibleRows() + first_visible - 1;

    SelectMode s = SELECT_NEXT;

    if(lines.constLast() < first_visible)
        s = SELECT_TOP;
    else if(lines.constFirst() > last_visible)
        s = SELECT_BOTTOM;

    if (keys.keyboardModifiers() & Qt::AltModifier)
    {
        if(lines.constFirst() == 0)
            return;

        int from = m_listWidget->model()->trackIndexAtLine(lines.constFirst());
        int to = m_listWidget->model()->trackIndexAtLine(lines.constFirst() - 1);

        if(from >= 0 && to >= 0)
        {
            m_listWidget->model()->moveTracks(from, to);
            m_listWidget->setAnchorLine(lines.constFirst() - 1);
        }
    }
    else if(keys.keyboardModifiers() & Qt::ControlModifier)
    {
        m_listWidget->setAnchorLine(qMax(m_listWidget->anchorLine() - 1, 0));
    }
    else
    {
        if(s == SELECT_TOP)
        {
            m_listWidget->model()->setSelectedLine(first_visible, true);
            m_listWidget->setAnchorLine(first_visible);
        }
        else if(s == SELECT_BOTTOM)
        {
            m_listWidget->model()->setSelectedLine(last_visible, true);
            m_listWidget->setAnchorLine(last_visible);
        }
        else if(lines.constFirst() == 0)
        {
            m_listWidget->model()->setSelectedLine(lines.constFirst(), true);
            m_listWidget->setAnchorLine(lines.constFirst());
        }
        else if(lines.contains(m_listWidget->anchorLine()) || m_listWidget->anchorLine() < 0)
        {
            m_listWidget->model()->setSelectedLine(lines.constFirst() - 1, true);
            m_listWidget->setAnchorLine(lines.constFirst() - 1);
        }
        else if(m_listWidget->anchorLine() >= 0)
        {
            m_listWidget->model()->setSelectedLine(m_listWidget->anchorLine(), true);
        }
    }

    if(m_listWidget->anchorLine() < first_visible)
    {
        m_listWidget->setViewPosition(m_listWidget->model()->firstSelectedLine());
    }
}

void QSUiKeyboardManager::processDown()
{
    if(!m_listWidget || m_listWidget->filterMode())
        return;

    QKeyCombination keys = qobject_cast<QAction *>(sender())->shortcut()[0];

    QList<int> lines = m_listWidget->model()->selectedLines();

    if(lines.isEmpty())
    {
        m_listWidget->model()->setSelectedLine(m_listWidget->firstVisibleLine(), true);
        m_listWidget->setAnchorLine(m_listWidget->firstVisibleLine());
        return;
    }

    if(!(keys.keyboardModifiers() & Qt::ShiftModifier || keys.keyboardModifiers() & Qt::AltModifier ||
         keys.keyboardModifiers() & Qt::ControlModifier))
    {
        m_listWidget->model()->clearSelection();
        m_listWidget->setAnchorLine(-1);
    }

    int first_visible = m_listWidget->firstVisibleLine();
    int last_visible = m_listWidget->visibleRows() + first_visible - 1;

    SelectMode s = SELECT_NEXT;

    if(lines.constLast() < first_visible)
        s = SELECT_TOP;
    else if(lines.constFirst() > last_visible)
        s = SELECT_BOTTOM;

    if (keys.keyboardModifiers() & Qt::AltModifier)
    {
        if(lines.constLast() == m_listWidget->model()->lineCount() - 1)
            return;

        int from = m_listWidget->model()->trackIndexAtLine(lines.constFirst());
        int to = m_listWidget->model()->trackIndexAtLine(lines.constFirst() + 1);

        if(from >= 0 && to >= 0)
        {
            m_listWidget->model()->moveTracks(from, to);
            m_listWidget->setAnchorLine(lines.constLast() + 1);
        }
    }
    else if(keys.keyboardModifiers() & Qt::ControlModifier)
    {
        m_listWidget->setAnchorLine(qMin(m_listWidget->anchorLine() + 1,
                                          m_listWidget->model()->lineCount() - 1));
    }
    else
    {
        if(s == SELECT_TOP)
        {
            m_listWidget->model()->setSelectedLine(first_visible, true);
            m_listWidget->setAnchorLine(first_visible);
        }
        else if(s == SELECT_BOTTOM)
        {
            m_listWidget->model()->setSelectedLine(last_visible, true);
            m_listWidget->setAnchorLine(last_visible);
        }
        else if(lines.constLast() == m_listWidget->model()->lineCount() - 1)
        {
            m_listWidget->model()->setSelectedLine(lines.constLast(), true);
            m_listWidget->setAnchorLine(lines.constLast());
        }
        else if(lines.contains(m_listWidget->anchorLine()) || m_listWidget->anchorLine() < 0)
        {
            m_listWidget->model()->setSelectedLine(lines.constLast() + 1, true);
            m_listWidget->setAnchorLine(lines.constLast() + 1);
        }
        else if(m_listWidget->anchorLine() >= 0)
        {
            m_listWidget->model()->setSelectedLine(m_listWidget->anchorLine(), true);
        }
    }

    if(m_listWidget->anchorLine() > last_visible)
    {
        m_listWidget->setViewPosition(m_listWidget->model()->lastSelectedLine(), true);
    }
}

void QSUiKeyboardManager::setListWidget(QSUiListWidget *listWidget)
{
    m_listWidget = listWidget;
}

void QSUiKeyboardManager::processEnter()
{
    if(!m_listWidget || m_listWidget->filterMode())
        return;
    QList<int> rows = m_listWidget->model()->selectedTrackIndexes();
    if(rows.isEmpty())
        return;
    MediaPlayer::instance()->stop();
    PlayListManager::instance()->activatePlayList(m_listWidget->model());
    m_listWidget->model()->setCurrent(rows.constFirst());
    MediaPlayer::instance()->play();
}

void QSUiKeyboardManager::processPgUp()
{
    if(!m_listWidget || m_listWidget->filterMode())
        return;

    int first = m_listWidget->firstVisibleLine();
    int offset = qMax(m_listWidget->firstVisibleLine() - m_listWidget->visibleRows(), 0);
    m_listWidget->setViewPosition(offset);

    m_listWidget->model()->clearSelection();
    if(m_listWidget->firstVisibleLine() == first)
        m_listWidget->setAnchorLine(0);
    else
        m_listWidget->setAnchorLine(m_listWidget->firstVisibleLine() + m_listWidget->visibleRows() / 2);
    m_listWidget->model()->setSelectedLine(m_listWidget->anchorLine(), true);
}

void QSUiKeyboardManager::processPgDown()
{
    if(!m_listWidget || m_listWidget->filterMode())
        return;

    int first = m_listWidget->firstVisibleLine();
    int offset = qMin(first + m_listWidget->visibleRows(), m_listWidget->model()->lineCount() - 1);
    m_listWidget->setViewPosition(offset);

    m_listWidget->model()->clearSelection();
    if(m_listWidget->firstVisibleLine() == first)
        m_listWidget->setAnchorLine(m_listWidget->model()->lineCount() - 1);
    else
        m_listWidget->setAnchorLine(m_listWidget->firstVisibleLine() + m_listWidget->visibleRows() / 2);
    m_listWidget->model()->setSelectedLine(m_listWidget->anchorLine(), true);
}

void QSUiKeyboardManager::processHome()
{
    if(!m_listWidget || m_listWidget->filterMode())
        return;
    QKeyCombination keys = qobject_cast<QAction *>(sender())->shortcut()[0];
    m_listWidget->setViewPosition(0);
    if(keys.keyboardModifiers() & Qt::ShiftModifier)
    {
        m_listWidget->model()->setSelectedLines(0, m_listWidget->anchorLine(), true);
    }
    else if(!m_listWidget->model()->isEmpty())
    {
        m_listWidget->model()->clearSelection();
        m_listWidget->setAnchorLine(0);
        m_listWidget->model()->setSelectedLine(0, true);
    }
}

void QSUiKeyboardManager::processEnd()
{
    if(!m_listWidget || m_listWidget->filterMode())
        return;

    QKeyCombination keys = qobject_cast<QAction *>(sender())->shortcut()[0];
    int scroll_to = m_listWidget->model()->lineCount() - m_listWidget->visibleRows();
    if(scroll_to >= 0)
        m_listWidget->setViewPosition(scroll_to);

    if(keys.keyboardModifiers() & Qt::ShiftModifier)
    {
        m_listWidget->model()->setSelectedLines(m_listWidget->anchorLine(), m_listWidget->model()->lineCount() - 1, true);
    }
    else if(!m_listWidget->model()->isEmpty())
    {
        m_listWidget->model()->clearSelection();
        m_listWidget->setAnchorLine(m_listWidget->model()->lineCount() - 1);
        m_listWidget->model()->setSelectedLine(m_listWidget->anchorLine(), true);
    }
}

template <typename Func1>
inline void QSUiKeyboardManager::addAction(QKeyCombination keys, Func1 slot)
{
    QAction *action = new QAction(this);
    action->setShortcut(QKeySequence(keys));
    connect(action, &QAction::triggered, this, slot);
    m_actions << action;
}
