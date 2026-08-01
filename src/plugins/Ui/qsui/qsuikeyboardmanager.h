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

#ifndef QSUIKEYBOARDMANAGER_H
#define QSUIKEYBOARDMANAGER_H

#include <QObject>
#include <QList>

class QAction;
class QSUiListWidget;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class QSUiKeyboardManager : public QObject
{
    Q_OBJECT
public:
    explicit QSUiKeyboardManager(QObject *parent = nullptr);

    QList<QAction*> actions();
    void setListWidget(QSUiListWidget *listWidget);

private slots:
    void processUp();
    void processDown();
    void processEnter();
    void processPgUp();
    void processPgDown();
    void processHome();
    void processEnd();

private:
    QList<QAction*> m_actions;
    QSUiListWidget *m_listWidget = nullptr;

    template <typename Func1>
    void addAction(QKeyCombination keys, Func1 slot);

    enum SelectMode
    {
        SELECT_TOP = 0,
        SELECT_BOTTOM,
        SELECT_NEXT
    };

};

#endif // QSUIKEYBOARDMANAGER_H
