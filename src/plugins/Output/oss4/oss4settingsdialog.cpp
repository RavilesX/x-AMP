/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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

extern "C"
{
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#else
#include <soundcard.h>
#endif
//#include </usr/lib/oss/include/sys/soundcard.h>
}
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "outputoss4.h"
#include "ui_oss4settingsdialog.h"
#include "oss4settingsdialog.h"

Oss4SettingsDialog::Oss4SettingsDialog (QWidget *parent) : QDialog (parent), m_ui(new Ui::Oss4SettingsDialog)
{
    m_ui->setupUi(this);

    int mixer_fd = -1;
    oss_sysinfo info;
    if ((mixer_fd = ::open(DEFAULT_MIXER, O_RDWR)) < 0)
    {
        qCWarning(plugin) << strerror(errno);
        return;
    }
    if (ioctl(mixer_fd, SNDCTL_SYSINFO, &info) < 0)
    {
        qCWarning(plugin, "ioctl SNDCTL_SYSINFO failed: %s", strerror(errno));
        return;
    }

    if (info.numaudios < 1)
    {
        qCWarning(plugin, "no device found");
        return;
    }

    m_devices << QStringLiteral(DEFAULT_DEV);
    m_ui->deviceComboBox->addItem(tr("Default (%1)").arg(QStringLiteral(DEFAULT_DEV)));

    for (int i = 0; i < info.numaudios; ++i)
    {
        oss_audioinfo audio_info;
        audio_info.dev = i;

        if (ioctl(mixer_fd, SNDCTL_AUDIOINFO, &audio_info) < 0)
        {
            qCWarning(plugin, "ioctl SNDCTL_AUDIOINFO failed: %s", strerror(errno));
            return;
        }

        if (audio_info.caps & PCM_CAP_OUTPUT)
        {
            m_devices << QString::fromLatin1(audio_info.devnode);
            m_ui->deviceComboBox->addItem(QStringLiteral("%1 (%2)").arg(QString::fromLatin1(audio_info.name),
                                                                        QString::fromLatin1(audio_info.devnode)));
        }
    }
    QSettings settings;
    m_ui->deviceComboBox->setEditText(settings.value(u"OSS4/device"_s, QStringLiteral(DEFAULT_DEV)).toString());
    connect(m_ui->deviceComboBox, &QComboBox::activated, this, &Oss4SettingsDialog::setText);
}

Oss4SettingsDialog::~Oss4SettingsDialog()
{
    delete m_ui;
}

void Oss4SettingsDialog::setText(int n)
{
    m_ui->deviceComboBox->setEditText(m_devices.at(n));
}

void Oss4SettingsDialog::accept()
{
    qCDebug(plugin) << Q_FUNC_INFO;
    QSettings settings;
    settings.setValue(u"OSS4/device"_s, m_ui->deviceComboBox->currentText());
    QDialog::accept();
}
