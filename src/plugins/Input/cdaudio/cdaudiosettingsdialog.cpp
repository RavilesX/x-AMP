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
#include <QDir>
#include <qmmp/qmmp.h>
#include "decoder_cdaudio.h"
#include "ui_cdaudiosettingsdialog.h"
#include "cdaudiosettingsdialog.h"

CDAudioSettingsDialog::CDAudioSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::CDAudioSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    settings.beginGroup(u"cdaudio"_s);
    m_ui->deviceLineEdit->setText(settings.value(u"device"_s).toString());
    m_ui->deviceCheckBox->setChecked(!m_ui->deviceLineEdit->text().isEmpty());
    int speed = settings.value(u"speed"_s, 0).toInt();
    m_ui->speedCheckBox->setChecked(speed > 0);
    m_ui->speedSpinBox->setValue(speed);
    m_ui->cdtextCheckBox->setChecked(settings.value(u"cdtext"_s, true).toBool());
#ifdef WITH_LIBCDDB
    m_ui->cddbGroupBox->setChecked(settings.value(u"use_cddb"_s, false).toBool());
    m_ui->httpCheckBox->setChecked(settings.value(u"cddb_http"_s, false).toBool());
    m_ui->serverLineEdit->setText(settings.value(u"cddb_server"_s, u"gnudb.org"_s).toString());
    m_ui->pathLineEdit->setText(settings.value(u"cddb_path"_s).toString());
    m_ui->portLineEdit->setText(settings.value(u"cddb_port"_s, 8880).toString());
#else
    m_ui->cddbGroupBox->setVisible(false);
    resize(width(), sizeHint().height());
#endif
    settings.endGroup();
}

CDAudioSettingsDialog::~CDAudioSettingsDialog()
{
    delete m_ui;
}

void CDAudioSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"cdaudio"_s);
    if(m_ui->deviceCheckBox->isChecked())
        settings.setValue(u"device"_s, m_ui->deviceLineEdit->text());
    else
        settings.remove(u"device"_s);
    if(m_ui->speedCheckBox->isChecked())
        settings.setValue(u"speed"_s, m_ui->speedSpinBox->value());
    else
        settings.setValue(u"speed"_s, 0);
    settings.setValue(u"cdtext"_s, m_ui->cdtextCheckBox->isChecked());
    settings.setValue(u"cdtext"_s, m_ui->cdtextCheckBox->isChecked());
#ifdef WITH_LIBCDDB
    settings.setValue(u"use_cddb"_s, m_ui->cddbGroupBox->isChecked());
    settings.setValue(u"cddb_http"_s, m_ui->httpCheckBox->isChecked());
    settings.setValue(u"cddb_server"_s,  m_ui->serverLineEdit->text());
    settings.setValue(u"cddb_path"_s, m_ui->pathLineEdit->text());
    settings.setValue(u"cddb_port"_s, m_ui->portLineEdit->text());
#endif
    settings.endGroup();
    settings.sync();
    DecoderCDAudio::clearTrackCache();
    QDialog::accept();
}

void CDAudioSettingsDialog::on_clearCacheButton_clicked()
{
    QDir dir(Qmmp::cacheDir());
    dir.cd(u"cddbcache"_s);
    const QStringList list = dir.entryList({ u"*"_s }, QDir::Files);
    for(const QString &name : std::as_const(list))
        dir.remove(name);
}
