/***************************************************************************
 *   Copyright (C) 2018-2026 by Ilya Kotov                                 *
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
#include "ui_rdetectsettingsdialog.h"
#include "rdetectsettingsdialog.h"

RDetectSettingsDialog::RDetectSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::RDetectSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    settings.beginGroup("rdetect"_L1);
    m_ui->cdGroupBox->setChecked(settings.value("cda"_L1, true).toBool());
    m_ui->addTracksCheckBox->setChecked(settings.value("add_tracks"_L1, false).toBool());
    m_ui->removeTracksCheckBox->setChecked(settings.value("remove_tracks"_L1, false).toBool());
    m_ui->removableGroupBox->setChecked(settings.value("removable"_L1, true).toBool());
    m_ui->addFilesCheckBox->setChecked(settings.value("add_files"_L1, false).toBool());
    m_ui->removeFilesCheckBox->setChecked(settings.value("remove_files"_L1, false).toBool());
    settings.endGroup();
}

RDetectSettingsDialog::~RDetectSettingsDialog()
{
    delete m_ui;
}

void RDetectSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup("rdetect"_L1);
    settings.setValue("cda"_L1, m_ui->cdGroupBox->isChecked());
    settings.setValue("add_tracks"_L1, m_ui->addTracksCheckBox->isChecked());
    settings.setValue("remove_tracks"_L1, m_ui->removeTracksCheckBox->isChecked());
    settings.setValue("removable"_L1, m_ui->removableGroupBox->isChecked());
    settings.setValue("add_files"_L1, m_ui->addFilesCheckBox->isChecked());
    settings.setValue("remove_files"_L1, m_ui->removeFilesCheckBox->isChecked());
    settings.endGroup();
    QDialog::accept();
}
