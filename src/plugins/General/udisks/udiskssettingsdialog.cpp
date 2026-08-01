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
#include <qmmp/qmmp.h>
#include "ui_udiskssettingsdialog.h"
#include "udiskssettingsdialog.h"

UDisksSettingsDialog::UDisksSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::UDisksSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    settings.beginGroup(u"UDisks"_s);
    m_ui->cdGroupBox->setChecked(settings.value(u"cda"_s, true).toBool());
    m_ui->addTracksCheckBox->setChecked(settings.value(u"add_tracks"_s, false).toBool());
    m_ui->removeTracksCheckBox->setChecked(settings.value(u"remove_tracks"_s, false).toBool());
    m_ui->removableGroupBox->setChecked(settings.value(u"removable"_s, true).toBool());
    m_ui->addFilesCheckBox->setChecked(settings.value(u"add_files"_s, false).toBool());
    m_ui->removeFilesCheckBox->setChecked(settings.value(u"remove_files"_s, false).toBool());
    settings.endGroup();
}


UDisksSettingsDialog::~UDisksSettingsDialog()
{
    delete m_ui;
}

void UDisksSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"UDisks"_s);
    settings.setValue(u"cda"_s, m_ui->cdGroupBox->isChecked());
    settings.setValue(u"add_tracks"_s, m_ui->addTracksCheckBox->isChecked());
    settings.setValue(u"remove_tracks"_s, m_ui->removeTracksCheckBox->isChecked());
    settings.setValue(u"removable"_s, m_ui->removableGroupBox->isChecked());
    settings.setValue(u"add_files"_s, m_ui->addFilesCheckBox->isChecked());
    settings.setValue(u"remove_files"_s, m_ui->removeFilesCheckBox->isChecked());
    settings.endGroup();
    QDialog::accept();
}
