/***************************************************************************
 *   Copyright (C) 2026 by x-AMP contributors                              *
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

#include <QApplication>
#include <QAbstractButton>
#include <QDialog>
#include <QMouseEvent>
#include <QWindow>
#include "xuidialogs.h"

namespace
{
    //marks a dialog already dealt with, so re-showing it does not loop
    const char *HANDLED = "xui_frameless";
}

void XUiDialogs::install()
{
    static XUiDialogs *filter = nullptr;
    if(filter)
        return;
    filter = new XUiDialogs(qApp);
    qApp->installEventFilter(filter);
}

void XUiDialogs::stripLabelledIcons(QWidget *root)
{
    const QList<QAbstractButton *> buttons = root->findChildren<QAbstractButton *>();
    for(QAbstractButton *button : buttons)
    {
        //an icon-only button would be left blank, so those keep theirs
        if(!button->icon().isNull() && !button->text().isEmpty())
            button->setIcon(QIcon());
    }
}

bool XUiDialogs::eventFilter(QObject *watched, QEvent *event)
{
    QDialog *dialog = qobject_cast<QDialog *>(watched);
    if(!dialog)
        return QObject::eventFilter(watched, event);

    if(event->type() == QEvent::Show && !dialog->property(HANDLED).toBool())
    {
        dialog->setProperty(HANDLED, true);
        stripLabelledIcons(dialog);
        //setWindowFlags hides the widget, so it has to be shown again; doing
        //this on Show rather than at construction keeps it to one frame and
        //works for dialogs built elsewhere
        dialog->setWindowFlags(dialog->windowFlags() | Qt::FramelessWindowHint);
        dialog->show();
        return false;
    }

    //Without a title bar there is nothing to drag, so a press on the dialog's
    //own background moves it. Presses on child widgets never reach here.
    if(event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
        if(mouse->button() == Qt::LeftButton && dialog->windowHandle())
        {
            dialog->windowHandle()->startSystemMove();
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}
