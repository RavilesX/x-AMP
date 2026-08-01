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
#ifndef BS2BSETTINGSDIALOG_H
#define BS2BSETTINGSDIALOG_H

#include <QDialog>
#include <bs2b/bs2b.h>

namespace Ui {
class Bs2bSettingsDialog;
}

/**
	@author Ilya Kotov <forkotov02@ya.ru>
*/
class Bs2bSettingsDialog : public QDialog
{
Q_OBJECT
public:
    explicit Bs2bSettingsDialog(QWidget *parent = nullptr);

    ~Bs2bSettingsDialog();

public slots:
    void accept() override;
    void reject() override;

private slots:
    void on_freqSlider_valueChanged(int value);
    void on_feedSlider_valueChanged(int value);

private:
    Ui::Bs2bSettingsDialog *m_ui;
    uint32_t m_level;

};

#endif
