/***************************************************************************
 *   Copyright (C) 2007-2026 by Ilya Kotov                                 *
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
#include <qmmp/visual.h>
#include <qmmp/visualfactory.h>
#include "visualmenu.h"

VisualMenu::VisualMenu(QWidget *parent) : QMenu(tr("Visualization"), parent)
{
    for(VisualFactory *factory : Visual::factories())
    {
        QAction *act = new QAction(factory->properties().name, this);
        act->setCheckable(true);
        act->setChecked(Visual::isEnabled(factory));
        connect(act, &QAction::triggered, this, [=] (bool checked) { Visual::setEnabled(factory, checked); });
        addAction(act);
    }
}

void VisualMenu::updateActions()
{
    for(int i = 0; i < Visual::factories().size(); ++i)
    {
        actions().at(i)->setChecked(Visual::isEnabled(Visual::factories().at(i)));
    }
}
