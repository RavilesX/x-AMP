/***************************************************************************
 *   Copyright (C) 2017-2026 by Ilya Kotov                                 *
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
#include "shoutsettingsdialog.h"
#include "ui_shoutsettingsdialog.h"

ShoutSettingsDialog::ShoutSettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::ShoutSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    settings.beginGroup(u"Shout"_s);
    m_ui->hostLineEdit->setText(settings.value(u"host"_s, u"127.0.0.1"_s).toString());
    m_ui->portSpinBox->setValue(settings.value(u"port"_s, 8000).toInt());
    m_ui->mountPointLineEdit->setText(settings.value(u"mount"_s, u"xamp.out"_s).toString());
    m_ui->userLineEdit->setText(settings.value(u"user"_s, u"source"_s).toString());
    m_ui->passwLineEdit->setText(settings.value(u"passw"_s, u"hackme"_s).toString());
    m_ui->publicCheckBox->setChecked(settings.value(u"public"_s, false).toBool());
    m_ui->qualitySpinBox->setValue(settings.value(u"vorbis_quality"_s, 0.8).toDouble());
    m_ui->srateSpinBox->setValue(settings.value(u"sample_rate"_s, 44100).toInt());
    m_ui->nameLineEdit->setText(settings.value(u"name"_s).toString());
    m_ui->urlLineEdit->setText(settings.value(u"url"_s).toString());
    m_ui->genreLineEdit->setText(settings.value(u"genre"_s).toString());
    m_ui->descriptionLineEdit->setText(settings.value(u"description"_s).toString());
    settings.endGroup();
}

ShoutSettingsDialog::~ShoutSettingsDialog()
{
    delete m_ui;
}

void ShoutSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"Shout"_s);
    settings.setValue(u"host"_s,  m_ui->hostLineEdit->text());
    settings.setValue(u"port"_s, m_ui->portSpinBox->value());
    settings.setValue(u"mount"_s, m_ui->mountPointLineEdit->text());
    settings.setValue(u"user"_s, m_ui->userLineEdit->text());
    settings.setValue(u"passw"_s, m_ui->passwLineEdit->text());
    settings.setValue(u"public"_s, m_ui->publicCheckBox->isChecked());
    settings.setValue(u"vorbis_quality"_s, m_ui->qualitySpinBox->value());
    settings.setValue(u"sample_rate"_s, m_ui->srateSpinBox->value());
    settings.setValue(u"name"_s, m_ui->nameLineEdit->text());
    settings.setValue(u"url"_s, m_ui->urlLineEdit->text());
    settings.setValue(u"genre"_s, m_ui->genreLineEdit->text());
    settings.setValue(u"description"_s, m_ui->descriptionLineEdit->text());
    settings.endGroup();
    QDialog::accept();
}
