/***************************************************************************
 *   Copyright (C) 2007 by Zhuravlev Uriy                                  *
 *   stalkerg@gmail.com                                                    *
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
#include <qmmp/qmmp.h>
#include "ui_osssettingsdialog.h"
#include "osssettingsdialog.h"

OssSettingsDialog::OssSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::OssSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    settings.beginGroup(u"OSS"_s);
    m_ui->lineEdit->insert(settings.value(u"device"_s, u"/dev/dsp"_s).toString());
    m_ui->lineEdit_2->insert(settings.value(u"mixer_device"_s, u"/dev/mixer"_s).toString());
    m_ui->bufferSpinBox->setValue(settings.value(u"buffer_time"_s, 500).toInt());
    m_ui->periodSpinBox->setValue(settings.value(u"period_time"_s, 100).toInt());
    settings.endGroup();
}

OssSettingsDialog::~OssSettingsDialog()
{
    delete m_ui;
}

void OssSettingsDialog::accept()
{
    qCDebug(plugin) << Q_FUNC_INFO;
    QSettings settings;
    settings.beginGroup(u"OSS"_s);
    settings.setValue(u"device"_s, m_ui->lineEdit->text());
    settings.setValue(u"buffer_time"_s, m_ui->bufferSpinBox->value());
    settings.setValue(u"period_time"_s, m_ui->periodSpinBox->value());
    settings.setValue(u"mixer_device"_s, m_ui->lineEdit_2->text());
    settings.endGroup();
    QDialog::accept();
}
