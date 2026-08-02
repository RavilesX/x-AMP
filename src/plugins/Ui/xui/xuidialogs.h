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

#ifndef XUIDIALOGS_H
#define XUIDIALOGS_H

#include <QObject>

class QWidget;

/*!
 * Drops the window manager's frame from this application's dialogs.
 *
 * A dialog's title bar is drawn by the window manager, so neither the palette
 * nor the style sheet reaches it and it stays the desktop's grey next to the
 * dark cards. Asking for the theme's dark variant with _GTK_THEME_VARIANT was
 * tried first and Muffin ignores it outright, so the frame goes instead: the
 * dialogs already carry an accent border of their own, and every one of them
 * has its own buttons to close with.
 *
 * Done from an application-wide filter rather than at each call site because
 * the dialogs that matter -- track details, preferences -- are built inside
 * libqmmpui, where this interface has no constructor to hook.
 */
class XUiDialogs : public QObject
{
    Q_OBJECT
public:
    explicit XUiDialogs(QObject *parent = nullptr) : QObject(parent) {}

    /*! Installs the filter. Safe to call more than once. */
    static void install();

    /*!
     * Drops the icon from every button in \b root that also carries a label.
     *
     * Those come from the desktop's icon theme and sit badly next to plain
     * text in this interface. Buttons that are nothing but an icon keep it,
     * since removing it would leave them blank.
     */
    static void stripLabelledIcons(QWidget *root);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif
