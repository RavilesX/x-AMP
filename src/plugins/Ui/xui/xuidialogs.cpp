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
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QWindow>
#include "xuitheme.h"
#include "xuiicons.h"
#include "xuidialogs.h"

namespace
{
    //marks a dialog already dealt with, so re-showing it does not loop
    const char *HANDLED = "xui_frameless";
    //the settings dialog's list of pages, named in configdialog.ui
    const char *PAGE_LIST = "contentsWidget";

    /*!
     * The page list of the settings dialog.
     *
     * Filling the selected row with the accent put white text on whatever
     * colour the user had picked, and against a light one the label vanished.
     * The row is marked with an accent chevron in the gutter instead, and the
     * fill stays a neutral grey, so the text is legible under any accent.
     */
    class PageDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;

    protected:
        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
        {
            QStyleOptionViewItem opt(option);
            const bool selected = opt.state & QStyle::State_Selected;
            const bool hovered = opt.state & QStyle::State_MouseOver;

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            if(selected || hovered)
            {
                painter->setPen(Qt::NoPen);
                painter->setBrush(selected ? XUi::Border : XUi::Hover);
                painter->drawRoundedRect(QRectF(opt.rect).adjusted(3, 2, -3, -2), 8, 8);
            }
            if(selected)
            {
                const qreal size = 20.0;
                XUiIcons::paint(painter, XUiIcons::ChevronRight,
                                QRectF(opt.rect.left() + 3.0,
                                       opt.rect.center().y() - size / 2.0, size, size),
                                XUi::Accent);
            }
            painter->restore();

            //the fill is drawn above, so the style must not draw its own
            opt.state &= ~(QStyle::State_Selected | QStyle::State_MouseOver);
            opt.palette.setColor(QPalette::HighlightedText, XUi::Text);
            opt.palette.setColor(QPalette::Text, XUi::Text);
            QStyledItemDelegate::paint(painter, opt, index);
        }
    };

    /*!
     * Keeps one page always marked.
     *
     * A click on the empty strip below the last row cleared the selection: the
     * page stayed on screen with nothing pointing at it. Presses that land on
     * no row are dropped.
     */
    class KeepSelection : public QObject
    {
    public:
        explicit KeepSelection(QListWidget *list) : QObject(list), m_list(list)
        {
            m_list->viewport()->installEventFilter(this);
        }

    protected:
        bool eventFilter(QObject *, QEvent *event) override
        {
            switch(event->type())
            {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseButtonRelease:
            {
                const QPoint pos = static_cast<QMouseEvent *>(event)
                                   ->position().toPoint();
                return !m_list->indexAt(pos).isValid();
            }
            default:
                return false;
            }
        }

    private:
        QListWidget *m_list;
    };
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
        if(QListWidget *pages = dialog->findChild<QListWidget *>(QLatin1String(PAGE_LIST)))
        {
            pages->setItemDelegate(new PageDelegate(pages));
            new KeepSelection(pages); //owned by the list
        }
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
