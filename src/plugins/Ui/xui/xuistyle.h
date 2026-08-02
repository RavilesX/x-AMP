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

#ifndef XUISTYLE_H
#define XUISTYLE_H

#include <QProxyStyle>

/*!
 * Style tweaks that a style sheet cannot express.
 *
 * Tooltip timing is a style hint, not a property, so the only way to change
 * it is through the style. Qt waits 700 ms before showing one, which is a
 * long time on controls that are icons with no label.
 */
class XUiStyle : public QProxyStyle
{
    Q_OBJECT
public:
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;
};

#endif
