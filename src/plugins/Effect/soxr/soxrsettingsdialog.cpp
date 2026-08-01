/***************************************************************************
 *   Copyright (C) 2016-2026 by Ilya Kotov                                 *
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
#include <soxr.h>
#include <qmmp/qmmp.h>
#include "ui_soxrsettingsdialog.h"
#include "soxrsettingsdialog.h"

SoXRSettingsDialog::SoXRSettingsDialog(QWidget *parent)
 : QDialog(parent), m_ui(new Ui::SoXRSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    m_ui->srSpinBox->setValue(settings.value(u"SOXR/sample_rate"_s, 48000).toInt());

    m_ui->qualityComboBox->addItem(tr("Quick"), SOXR_QQ);
    m_ui->qualityComboBox->addItem(tr("Low"), SOXR_LQ);
    m_ui->qualityComboBox->addItem(tr("Medium"), SOXR_MQ);
    m_ui->qualityComboBox->addItem(tr("High"), SOXR_HQ);
    m_ui->qualityComboBox->addItem(tr("Very High"), SOXR_VHQ);
    int index = m_ui->qualityComboBox->findData(settings.value(u"SOXR/quality"_s, SOXR_HQ).toInt());
    if(index >= 0 && index < m_ui->qualityComboBox->count())
        m_ui->qualityComboBox->setCurrentIndex(index);
}


SoXRSettingsDialog::~SoXRSettingsDialog()
{
    delete m_ui;
}

void SoXRSettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(u"SOXR/sample_rate"_s, m_ui->srSpinBox->value());
    settings.setValue(u"SOXR/quality"_s, m_ui->qualityComboBox->itemData(m_ui->qualityComboBox->currentIndex()).toInt());
    QDialog::accept();
}
