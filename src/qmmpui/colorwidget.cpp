/***************************************************************************
 *   Copyright (C) 2005-2026 by Ilya Kotov                                 *
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

#include <QPalette>
#include "colorwidget.h"

class ColorWidgetPrivate
{
public:
    QString colorName;
    QColorDialog::ColorDialogOptions options;
};

ColorWidget::ColorWidget(QWidget *parent) :
    QFrame(parent),
    d_ptr(new ColorWidgetPrivate)
{
    setFrameShape(QFrame::Box);
    setAutoFillBackground(true);
}

ColorWidget::~ColorWidget()
{
    delete d_ptr;
}

QColorDialog::ColorDialogOptions ColorWidget::options() const
{
    return d_ptr->options;
}

void ColorWidget::setOptions(QColorDialog::ColorDialogOptions options)
{
    Q_D(ColorWidget);
    if(d->options != options)
    {
        d->options = options;
        emit optionsChanged();
    }
}

void ColorWidget::mousePressEvent(QMouseEvent *)
{
    Q_D(ColorWidget);
    QColor color = QColorDialog::getColor(QColor(d->colorName), parentWidget(), tr("Select Color"), d->options);
    if(color.isValid())
    {
        setColor(color.name((d->options & QColorDialog::ShowAlphaChannel) ? QColor::HexArgb : QColor::HexRgb));
    }
}

void ColorWidget::setColor(const QString &name)
{
    Q_D(ColorWidget);
    d->colorName = name;
    setStyleSheet(QStringLiteral("QFrame { background: %1 }").arg(d->colorName));
}

void ColorWidget::setColor(const QColor &color)
{
    setColor(color.name((d_ptr->options & QColorDialog::ShowAlphaChannel) ? QColor::HexArgb : QColor::HexRgb));
}

QString ColorWidget::colorName() const
{
    return d_ptr->colorName;
}

QColor ColorWidget::color() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    return QColor::fromString(d_ptr->colorName);
#else
    QColor color;
    color.setNamedColor(d_ptr->colorName);
    return color;
#endif
}
