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

#ifndef DOCKWIDGETLIST_H
#define DOCKWIDGETLIST_H

#include <QMainWindow>
#include <QList>

class QDockWidget;
class QAction;
class QMenu;

class DockWidgetList : public QObject
{
    Q_OBJECT
public:
    explicit DockWidgetList(QMainWindow *parent);

    void registerMenu(QMenu *menu, QAction *before);
    void setTitleBarsVisible(bool visible);

private slots:
    void onViewActionTriggered(bool checked);
    void onVisibilityChanged(bool visible);
    void onWidgetAdded(const QString &id);
    void onWidgetRemoved(const QString &id);
    void onWidgetUpdated(const QString &id);

private:
    QMainWindow *m_mw;
    QList<QDockWidget *> m_dockWidgetList;
    QAction *m_beforeAction = nullptr;
    QMenu *m_menu;
    bool m_titleBarsVisible = true;
};

#endif // DOCKWIDGETLIST_H
