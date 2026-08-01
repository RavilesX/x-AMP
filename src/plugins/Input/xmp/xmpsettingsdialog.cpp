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
#include <QDir>
#include <xmp.h>
#include "decoder_xmp.h"
#include "ui_xmpsettingsdialog.h"
#include "xmpsettingsdialog.h"

XmpSettingsDialog::XmpSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::XmpSettingsDialog)
{
    m_ui->setupUi(this);
    //prepare combobox
    m_ui->srateComboBox->addItem(tr("22050 Hz"), 22050);
    m_ui->srateComboBox->addItem(tr("44100 Hz"), 44100);
    m_ui->srateComboBox->addItem(tr("48000 Hz"), 48000);
    m_ui->intTypeComboBox->addItem(tr("Nearest Neighbor"), XMP_INTERP_NEAREST);
    m_ui->intTypeComboBox->addItem(tr("Linear"), XMP_INTERP_LINEAR);
    m_ui->intTypeComboBox->addItem(tr("Cubic Spline"), XMP_INTERP_SPLINE);
    //load settings
    QSettings settings;
    settings.beginGroup(u"Xmp"_s);
    m_ui->ampFactorSpinBox->setValue(settings.value(u"amp_factor"_s, 1).toInt());
    m_ui->stereoMixingSpinBox->setValue(settings.value(u"stereo_mix"_s, 70).toInt());
    int index = m_ui->intTypeComboBox->findData(settings.value(u"interpolation"_s, XMP_INTERP_LINEAR).toInt());
    if(index >= 0)
        m_ui->intTypeComboBox->setCurrentIndex(index);
    index = m_ui->srateComboBox->findData(settings.value(u"sample_rate"_s, 44100).toInt());
    if(index >= 0)
        m_ui->srateComboBox->setCurrentIndex(index);
    m_ui->lowPassCheckBox->setChecked(settings.value(u"lowpass"_s, false).toBool());
    m_ui->vblankCheckBox->setChecked(settings.value(u"vblank"_s, false).toBool());
    m_ui->fx9BugCheckBox->setChecked(settings.value(u"fx9bug"_s, false).toBool());
    m_ui->fixLoopCheckBox->setChecked(settings.value(u"fixlopp"_s, false).toBool());
    m_ui->a500CheckBox->setChecked(settings.value(u"a500"_s, false).toBool());
    settings.endGroup();
}

XmpSettingsDialog::~XmpSettingsDialog()
{
    delete m_ui;
}

void XmpSettingsDialog::writeSettings()
{
    QSettings settings;
    settings.beginGroup(u"Xmp"_s);
    settings.setValue(u"amp_factor"_s, m_ui->ampFactorSpinBox->value());
    settings.setValue(u"stereo_mix"_s, m_ui->stereoMixingSpinBox->value());
    settings.setValue(u"interpolation"_s, m_ui->intTypeComboBox->currentData());
    settings.setValue(u"sample_rate"_s, m_ui->srateComboBox->currentData());
    settings.setValue(u"lowpass"_s, m_ui->lowPassCheckBox->isChecked());
    settings.setValue(u"vblank"_s, m_ui->vblankCheckBox->isChecked());
    settings.setValue(u"fx9bug"_s, m_ui->fx9BugCheckBox->isChecked());
    settings.setValue(u"fixlopp"_s, m_ui->fixLoopCheckBox->isChecked());
    settings.setValue(u"a500"_s, m_ui->a500CheckBox->isChecked());
    settings.endGroup();
    //apply settings for the created decoder
    if (DecoderXmp::instance())
    {
        //DecoderXmp::instance()->mutex()->lock();
        DecoderXmp::instance()->readSettings();
        //DecoderXmp::instance()->mutex()->unlock();
    }
}

void XmpSettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch ((int) m_ui->buttonBox->buttonRole(button))
    {
    case QDialogButtonBox::AcceptRole:
        writeSettings();
        accept();
        break;
    case QDialogButtonBox::ApplyRole:
        writeSettings();
        break;
    }
}
