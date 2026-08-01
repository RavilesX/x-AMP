/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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
#ifndef LADSPASETTINGSDIALOG_H
#define LADSPASETTINGSDIALOG_H

#include <QDialog>

class QStandardItemModel;

namespace Ui {
class LADSPASettingsDialog;
}

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class LADSPASettingsDialog : public QDialog
{
Q_OBJECT
public:
    explicit LADSPASettingsDialog(QWidget *parent = nullptr);

    ~LADSPASettingsDialog();

private slots:
    void on_loadButton_clicked();
    void on_unloadButton_clicked();
    void onConfigureButtonClicked();

private:
    void updateRunningPlugins();
    Ui::LADSPASettingsDialog *m_ui;
    QStandardItemModel *m_model;
};

#endif
