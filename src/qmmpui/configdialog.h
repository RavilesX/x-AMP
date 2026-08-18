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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QIcon>
#include "qmmpui_export.h"

class ConfigDialogPrivate;

/*! @brief Configuration dialog class.
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class QMMPUI_EXPORT ConfigDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ConfigDialog)
public:
    /*!
     * Constructor.
     * \param parent Parent widget
     */
    ConfigDialog(QWidget *parent = nullptr);
    /*!
     * Destructor
     */
    virtual ~ConfigDialog();
    /*!
     * Adds custom page in the configuration dialog
     * \param name Localized name of the custom page
     * \param widget Custom page instance
     * \param icon Custom page icon
     */
    void addPage(const QString &name, QWidget *widget, const QIcon &icon = QIcon());
    /*!
     * Opens the page holding the scheduler and scrolls it into view, so a
     * user interface can offer a direct way in instead of leaving the section
     * to be hunted for.
     */
    void showSchedulerSettings();

private:
    ConfigDialogPrivate *d_ptr;
};

#endif
