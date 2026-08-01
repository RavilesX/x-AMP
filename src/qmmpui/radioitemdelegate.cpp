/***************************************************************************
 *   Copyright (C) 2012-2026 by Ilya Kotov                                 *
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

#include <QApplication>
#include <QMouseEvent>
#include <QProxyStyle>
#include "radioitemdelegate_p.h"

class RadioItemStyle : public QProxyStyle
{
public:
    RadioItemStyle() : QProxyStyle() {};

    void drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override
    {
        if(element == QStyle::PE_IndicatorItemViewItemCheck)
            element = QStyle::PE_IndicatorRadioButton;

        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

RadioItemDelegate::RadioItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    m_style = new RadioItemStyle();
}

RadioItemDelegate::~RadioItemDelegate()
{
    delete m_style;
}

void RadioItemDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    if(hasRadioButton(index))
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        m_style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize RadioItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    if(hasRadioButton(index))
    {
        int buttonHeight = qApp->style()->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight, &option);
        size.setHeight(qMax(size.height(), buttonHeight));
    }
    return size;
}

bool RadioItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                    const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if(event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::KeyPress)
    {
        if(hasRadioButton(index))
        {
            if(event->type() == QEvent::MouseButtonRelease)
            {
                QRect checkRect = qApp->style()->subElementRect(QStyle::SE_RadioButtonIndicator, &option);

                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

                if(!checkRect.contains(mouseEvent->pos()))
                    return true;
            }

            if (!index.data(Qt::CheckStateRole).toBool())
            {
                model->setData(index, Qt::Checked, Qt::CheckStateRole);
                QModelIndex parentItem = index.parent();
                for (int i = 0; i < model->rowCount(parentItem); ++i)
                {
                    QModelIndex childIndex = model->index(i, 0, parentItem);

                    if (childIndex != index)
                        model->setData(childIndex, Qt::Unchecked, Qt::CheckStateRole);
                }
            }
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event,model,option,index);
}

bool RadioItemDelegate::hasRadioButton(const QModelIndex &index) const
{
    return index.flags().testFlag(Qt::ItemIsUserCheckable) && index.data(RadioButtonRole).toBool();
}
