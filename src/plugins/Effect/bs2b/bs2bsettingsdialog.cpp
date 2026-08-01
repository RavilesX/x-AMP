/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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
#include <QPushButton>
#include <bs2b/bs2b.h>
#include <qmmp/qmmp.h>
#include "ui_bs2bsettingsdialog.h"
#include "bs2bplugin.h"
#include "bs2bsettingsdialog.h"

Bs2bSettingsDialog::Bs2bSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::Bs2bSettingsDialog)
{
    m_ui->setupUi(this);
    m_ui->feedSlider->setRange(BS2B_MINFEED, BS2B_MAXFEED);
    m_ui->freqSlider->setRange(BS2B_MINFCUT, BS2B_MAXFCUT);
    QSettings settings;
    m_level = settings.value(u"bs2b/level"_s, BS2B_DEFAULT_CLEVEL).toUInt();
    m_ui->feedSlider->setValue(m_level >> 16);
    m_ui->freqSlider->setValue(m_level & 0xffff);

    connect(m_ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, [this] {
        m_ui->feedSlider->setValue(BS2B_DEFAULT_CLEVEL >> 16);
        m_ui->freqSlider->setValue(BS2B_DEFAULT_CLEVEL & 0xffff);
    });

    connect(m_ui->buttonBox->addButton(tr("C.Moy"), QDialogButtonBox::ResetRole), &QPushButton::clicked, this, [this] {
        m_ui->feedSlider->setValue(BS2B_CMOY_CLEVEL >> 16);
        m_ui->freqSlider->setValue(BS2B_CMOY_CLEVEL & 0xffff);
    });

    connect(m_ui->buttonBox->addButton(tr("J. Meier"), QDialogButtonBox::ResetRole), &QPushButton::clicked, this, [this] {
        m_ui->feedSlider->setValue(BS2B_JMEIER_CLEVEL >> 16);
        m_ui->freqSlider->setValue(BS2B_JMEIER_CLEVEL & 0xffff);
    });
}

Bs2bSettingsDialog::~Bs2bSettingsDialog()
{
    delete m_ui;
}

void Bs2bSettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(u"bs2b/level"_s, m_ui->feedSlider->value() << 16 | m_ui->freqSlider->value());
    QDialog::accept();
}

void Bs2bSettingsDialog::Bs2bSettingsDialog::reject()
{
    if (Bs2bPlugin::instance()) //restore crossfeed settings
        Bs2bPlugin::instance()->setCrossfeedLevel(m_level);
    QDialog::reject();
}

void Bs2bSettingsDialog::on_freqSlider_valueChanged(int value)
{
    m_ui->freqLabel->setText(tr("%1 Hz, %2 us").arg(value).arg(bs2b_level_delay(value)));
    if (Bs2bPlugin::instance())
        Bs2bPlugin::instance()->setCrossfeedLevel(m_ui->feedSlider->value() << 16 | m_ui->freqSlider->value());
}

void Bs2bSettingsDialog::on_feedSlider_valueChanged(int value)
{
    m_ui->feedLabel->setText(tr("%1 dB").arg((double)value / 10));
    if (Bs2bPlugin::instance())
        Bs2bPlugin::instance()->setCrossfeedLevel(m_ui->feedSlider->value() << 16 | m_ui->freqSlider->value());
}
