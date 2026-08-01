/***************************************************************************
 *   Copyright (C) 2015 by Ivan Ponomarev                                  *
 *   ivantrue@gmail.com                                                    *
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
#include <QAudioDevice>
#include <QMediaDevices>
#include <qmmp/qmmp.h>
#include "ui_qtmultimediasettingsdialog.h"
#include "qtmultimediasettingsdialog.h"

QtMultimediaSettingsDialog::QtMultimediaSettingsDialog (QWidget *parent) : QDialog (parent),
    m_ui(new Ui::QtMultimediaSettingsDialog)
{
    m_ui->setupUi(this);

	const QSettings settings;
    const QByteArray default_device = settings.value("QTMULTIMEDIA/device"_L1).toByteArray();

	//Default item always has index = 0
    m_ui->deviceComboBox->addItem(tr("Default"));
    m_ui->deviceComboBox->setCurrentIndex(0);

    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
	int index = 1;
    for(const QAudioDevice &info : devices)
	{
        m_ui->deviceComboBox->addItem(info.description(), info.id());
        if (info.id() == default_device)
            m_ui->deviceComboBox->setCurrentIndex(index);
		++index;
	}
}

QtMultimediaSettingsDialog::~QtMultimediaSettingsDialog()
{
    delete m_ui;
}

void QtMultimediaSettingsDialog::accept()
{
    qCDebug(plugin) << Q_FUNC_INFO;
    QSettings settings;
    settings.setValue("QTMULTIMEDIA/device"_L1, m_ui->deviceComboBox->currentData().toByteArray());
    QDialog::accept();
}
