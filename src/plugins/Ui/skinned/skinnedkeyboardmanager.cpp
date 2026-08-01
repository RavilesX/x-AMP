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

#include <QKeyEvent>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/mediaplayer.h>
#include "skinnedplaylist.h"
#include "skinnedlistwidget.h"
#include "skinnedkeyboardmanager.h"

SkinnedKeyboardManager::SkinnedKeyboardManager(SkinnedListWidget *l)
{
    m_listWidget = l;
}

bool SkinnedKeyboardManager::handleKeyPress(QKeyEvent* ke)
{
    bool handled = true;
    switch(ke->key())
    {
        case Qt::Key_Up:
            keyUp (ke);
            break;
        case Qt::Key_Down:
            keyDown (ke);
            break;
        case Qt::Key_PageUp:
            keyPgUp (ke);
            break;
        case Qt::Key_PageDown:
            keyPgDown (ke);
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            keyEnter (ke);
            break;
        case Qt::Key_Home:
            keyHome(ke);
            break;
         case Qt::Key_End:
            keyEnd(ke);
            break;
        default:
            handled = false;
    }
    return handled;
}

bool SkinnedKeyboardManager::handleKeyRelease (QKeyEvent*)
{
    return false;
}

void SkinnedKeyboardManager::keyUp(QKeyEvent * ke)
{
    QList<int> lines = m_listWidget->model()->selectedLines();

    if(lines.isEmpty())
    {
        m_listWidget->model()->setSelectedLine(m_listWidget->firstVisibleLine(), true);
        m_listWidget->setAnchorLine(m_listWidget->firstVisibleLine());
        return;
    }

    if (!(ke->modifiers() & Qt::ShiftModifier || ke->modifiers() & Qt::AltModifier ||
          ke->modifiers() & Qt::ControlModifier))
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

    if (ke->modifiers() & Qt::AltModifier)
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
    else if(ke->modifiers() & Qt::ControlModifier)
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

void SkinnedKeyboardManager::keyDown(QKeyEvent * ke)
{
    QList<int> lines = m_listWidget->model()->selectedLines();

    if(lines.isEmpty())
    {
        m_listWidget->model()->setSelectedLine(m_listWidget->firstVisibleLine(), true);
        m_listWidget->setAnchorLine(m_listWidget->firstVisibleLine());
        return;
    }

    if(!(ke->modifiers() & Qt::ShiftModifier || ke->modifiers() & Qt::AltModifier || ke->modifiers() & Qt::ControlModifier))
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

    if (ke->modifiers() & Qt::AltModifier)
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
    else if(ke->modifiers() & Qt::ControlModifier)
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

void SkinnedKeyboardManager::keyPgUp(QKeyEvent *)
{
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

void SkinnedKeyboardManager::keyPgDown(QKeyEvent *)
{
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

void SkinnedKeyboardManager::keyEnter(QKeyEvent *)
{    
    QList<int> rows = m_listWidget->model()->selectedTrackIndexes();
    if(rows.isEmpty())
        return;
    MediaPlayer::instance()->stop();
    PlayListManager::instance()->activatePlayList(m_listWidget->model());
    m_listWidget->model()->setCurrent(rows.constFirst());
    MediaPlayer::instance()->play();
}

void SkinnedKeyboardManager::keyHome(QKeyEvent *ke)
{
    m_listWidget->setViewPosition(0);
    if(ke->modifiers() & Qt::ShiftModifier)
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

void SkinnedKeyboardManager::keyEnd(QKeyEvent *ke)
{
    int scroll_to = m_listWidget->model()->lineCount() - m_listWidget->visibleRows();
    if(scroll_to >= 0)
        m_listWidget->setViewPosition(scroll_to);

    if(ke->modifiers() & Qt::ShiftModifier)
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
