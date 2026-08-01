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
#include <qmmp/qmmp.h>
#include "stereoplugin.h"
#include "ui_stereosettingsdialog.h"
#include "stereosettingsdialog.h"

StereoSettingsDialog::StereoSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::StereoSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    m_level = settings.value(u"extra_stereo/intensity"_s, 1.0).toDouble();
    m_ui->intensitySlider->setValue(m_level * 100 / 10.0);
}

StereoSettingsDialog::~StereoSettingsDialog()
{
    delete m_ui;
}

void StereoSettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(u"extra_stereo/intensity"_s, m_ui->intensitySlider->value() * 10.0 / 100);
    QDialog::accept();
}

void StereoSettingsDialog::StereoSettingsDialog::reject()
{
    if (StereoPlugin::instance()) //restore settings
        StereoPlugin::instance()->setIntensity(m_level);
    QDialog::reject();
}

void StereoSettingsDialog::on_intensitySlider_valueChanged (int value)
{
    double level = value * 10.0 / 100;
    m_ui->intensityLabel->setText(QString::number(level));
    if (StereoPlugin::instance())
        StereoPlugin::instance()->setIntensity(level);
}
