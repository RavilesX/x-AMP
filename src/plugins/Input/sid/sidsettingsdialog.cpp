/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
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
#include <QFileInfo>
#include <qmmp/qmmp.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/sidplayfp.h>
#include "sidsettingsdialog.h"
#include "ui_sidsettingsdialog.h"

SidSettingsDialog::SidSettingsDialog(SidDatabase *db, QWidget *parent) : QDialog(parent),
     m_ui(new Ui::SidSettingsDialog), m_db(db)
{
    m_ui->setupUi(this);

    QSettings settings;
    settings.beginGroup(u"SID"_s);

    m_ui->useHVSCCheckBox->setChecked(settings.value(u"use_hvsc"_s, false).toBool());
    QString hvsc_default_path = Qmmp::configDir() + u"/Songlengths.txt"_s;
    m_ui->hvscPathLineEdit->setText(settings.value(u"hvsc_path"_s, hvsc_default_path).toString());
    m_ui->defaultLengthSpinBox->setValue(settings.value(u"song_length"_s, 180).toInt());

    m_ui->sampleRateComboBox->addItem(tr("44100 Hz"), 44100);
    m_ui->sampleRateComboBox->addItem(tr("48000 Hz"), 48000);
    int index = m_ui->sampleRateComboBox->findData(settings.value(u"sample_rate"_s, 48000).toInt());
    m_ui->sampleRateComboBox->setCurrentIndex(index);

#if LIBSIDPLAYFP_VERSION_MAJ >= 3
    m_ui->emuComboBox->addItem(u"SIDLite"_s, u"sidlite"_s);
    m_ui->emuComboBox->addItem(u"ReSIDfp"_s, u"residfp"_s);
    index = m_ui->emuComboBox->findData(settings.value(u"engine"_s, u"sidlite"_s).toString());
    m_ui->emuComboBox->setCurrentIndex(qMax(index, 0));
    m_ui->fastResampligCheckBox->setVisible(false);
#else
    m_ui->emuComboBox->addItem(u"ReSID"_s, u"resid"_s);
    m_ui->emuComboBox->addItem(u"ReSIDfp"_s, u"residfp"_s);
    index = m_ui->emuComboBox->findData(settings.value(u"engine"_s, u"residfp"_s).toString());
    m_ui->emuComboBox->setCurrentIndex(qMax(index, 0));
    m_ui->fastResampligCheckBox->setChecked(settings.value(u"fast_resampling"_s, false).toBool());
#endif

//TODO use QFormLayout::setRowVisible
#ifndef HAVE_RESIDFP_HEADER
    m_ui->emulationLabel->setVisible(false);
    m_ui->emuComboBox->setVisible(false);
#endif

    m_ui->resamplingComboBox->addItem(u"Interpolate"_s, SidConfig::INTERPOLATE);
    m_ui->resamplingComboBox->addItem(u"Resample Interpolate"_s, SidConfig::RESAMPLE_INTERPOLATE);
    index = m_ui->resamplingComboBox->findData(settings.value(u"resampling_method"_s, SidConfig::INTERPOLATE).toInt());
    m_ui->resamplingComboBox->setCurrentIndex(index);

    settings.endGroup();
}

SidSettingsDialog::~SidSettingsDialog()
{
    delete m_ui;
}

void SidSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"SID"_s);
    settings.setValue(u"use_hvsc"_s, m_ui->useHVSCCheckBox->isChecked());
    settings.setValue(u"hvsc_path"_s, m_ui->hvscPathLineEdit->text());
    settings.setValue(u"song_length"_s, m_ui->defaultLengthSpinBox->value());
    int i = m_ui->sampleRateComboBox->currentIndex();
    if(i >= 0)
        settings.setValue(u"sample_rate"_s, m_ui->sampleRateComboBox->itemData(i));
    if((i = m_ui->emuComboBox->currentIndex()) >= 0)
        settings.setValue(u"engine"_s, m_ui->emuComboBox->itemData(i));
    settings.setValue(u"fast_resampling"_s, m_ui->fastResampligCheckBox->isChecked());
    if((i = m_ui->resamplingComboBox->currentIndex()) >= 0)
        settings.setValue(u"resampling_method"_s, m_ui->resamplingComboBox->itemData(i));
    m_db->close();
    if(m_ui->useHVSCCheckBox->isChecked())
    {
        if(!m_db->open(qPrintable(m_ui->hvscPathLineEdit->text())))
            qCWarning(plugin) << m_db->error();
    }
    settings.endGroup();
    QDialog::accept();
}
