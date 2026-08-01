/***************************************************************************
 *   Copyright (C) 2015-2026 by Ilya Kotov                                 *
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

#include <QSettings>
#include <qmmp/qmmp.h>
#include "gmesettingsdialog.h"
#include "ui_gmesettingsdialog.h"

GmeSettingsDialog::GmeSettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::GmeSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    m_ui->fadeoutCheckBox->setChecked(settings.value(u"GME/fadeout"_s, false).toBool());
    m_ui->fadeoutSpinBox->setValue(settings.value(u"GME/fadeout_length"_s, 7000).toInt());
}

GmeSettingsDialog::~GmeSettingsDialog()
{
    delete m_ui;
}

void GmeSettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(u"GME/fadeout"_s, m_ui->fadeoutCheckBox->isChecked());
    settings.setValue(u"GME/fadeout_length"_s, m_ui->fadeoutSpinBox->value());
    QDialog::accept();
}
