/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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
#ifndef SHORTCUTDIALOG_H
#define SHORTCUTDIALOG_H

#include <QDialog>
#include "qmmpui_export.h"

class QKeyEvent;

namespace Ui {
class ShortcutDialog;
}

class ShortcutDialogPrivate;

/*! @brief The ShortcutDialog class provides simple hotkey editor dialog.
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class QMMPUI_EXPORT ShortcutDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ShortcutDialog)
public:
    /*!
     * Object constructor.
     * \param key Initial shortcut string.
     * \param parent Parent object.
     */
    explicit ShortcutDialog(const QKeySequence &key, QWidget *parent = nullptr);
    /*!
     * Destructor
     */
    virtual ~ShortcutDialog();
    /*!
     * Returns assigned shortcut.
     */
    QKeySequence key() const;

protected:
    /*!
     * Reimplements \b QWidget::keyPressEvent(QKeyEvent *event)
     */
    virtual void keyPressEvent(QKeyEvent *event) override;
    /*!
     * Reimplements \b QWidget::keyPressEvent(QKeyEvent *event)
     */
    virtual void showEvent(QShowEvent *event) override;

private:
    ShortcutDialogPrivate *d_ptr;
};

#endif
