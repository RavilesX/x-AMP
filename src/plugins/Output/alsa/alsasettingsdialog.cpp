/***************************************************************************
 *   Copyright (C) 2006-2026 by Ilya Kotov                                 *
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
extern "C" {
#include <alsa/asoundlib.h>
}
#include "ui_alsasettingsdialog.h"
#include "alsasettingsdialog.h"

AlsaSettingsDialog::AlsaSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::AlsaSettingsDialog)
{
    m_ui->setupUi(this);
    m_ui->deviceComboBox->setEditable (true);
    getCards();
    getSoftDevices();
    connect(m_ui->deviceComboBox, &QComboBox::activated, this, &AlsaSettingsDialog::setText);
    connect(m_ui->mixerCardComboBox, &QComboBox::activated, this, &AlsaSettingsDialog::showMixerDevices);
    QSettings settings;
    settings.beginGroup(u"ALSA"_s);
    m_ui->deviceComboBox->setEditText(settings.value(u"device"_s, u"default"_s).toString());
    m_ui->bufferSpinBox->setValue(settings.value(u"buffer_time"_s, 500).toInt());
    m_ui->periodSpinBox->setValue(settings.value(u"period_time"_s, 100).toInt());

    int d = m_cards.indexOf(settings.value(u"mixer_card"_s, u"hw:0"_s).toString());
    if (d >= 0)
        m_ui->mixerCardComboBox->setCurrentIndex(d);

    showMixerDevices(m_ui->mixerCardComboBox->currentIndex ());
    d = m_ui->mixerDeviceComboBox->findText(settings.value(u"mixer_device"_s, u"PCM"_s).toString());

    if (d >= 0)
        m_ui->mixerDeviceComboBox->setCurrentIndex(d);

    m_ui->mmapCheckBox->setChecked(settings.value(u"use_mmap"_s, false).toBool());
    m_ui->pauseCheckBox->setChecked(settings.value(u"use_snd_pcm_pause"_s, false).toBool());
    settings.endGroup();
}


AlsaSettingsDialog::~AlsaSettingsDialog()
{
    delete m_ui;
}

void AlsaSettingsDialog::getCards()
{
    int card = -1, err;

    m_devices.clear();
    m_devices << u"default"_s;
    m_ui->deviceComboBox->addItem(u"Default PCM device (default)"_s);

    if((err = snd_card_next(&card)) !=0)
        qCWarning(plugin, "snd_next_card() failed: %s",
                 snd_strerror(-err));

    while(card > -1)
    {
        getCardDevices(card);
        m_cards << QStringLiteral("hw:%1").arg(card);
        if ((err = snd_card_next(&card)) !=0)
        {
            qCWarning(plugin, "snd_next_card() failed: %s",
                     snd_strerror(-err));
            break;
        }
    }
}

void AlsaSettingsDialog::getSoftDevices()
{
    void **hints = nullptr;
    int i = 0;

    if(snd_device_name_hint(-1, "pcm", &hints) < 0)
        return;

    while(hints && hints[i])
    {
        char *type = snd_device_name_get_hint (hints[i], "IOID");
        if (!type || !strcmp (type, "Output"))
        {
            char *device_name = snd_device_name_get_hint (hints[i], "NAME");
            char *device_desc = snd_device_name_get_hint (hints[i], "DESC");

            m_devices << QString::fromLatin1(device_name);
            QString str = QStringLiteral("%1 (%2)").arg(QString::fromLatin1(device_desc), QString::fromLatin1(device_name));
            qCDebug(plugin) << str;
            m_ui->deviceComboBox->addItem(str);
            free (device_name);
            free (device_desc);
        }
        if(type)
            free (type);
        ++i;
    }

    if (hints)
        snd_device_name_free_hint (hints);
}

void AlsaSettingsDialog::getCardDevices(int card)
{
    int pcm_device = -1, err;
    snd_pcm_info_t *pcm_info;
    snd_ctl_t *ctl;
    char dev[64], *card_name;

    sprintf(dev, "hw:%i", card);

    if ((err = snd_ctl_open(&ctl, dev, 0)) < 0)
    {
        qCWarning(plugin, "snd_ctl_open() failed: %s",
                 snd_strerror(-err));
        return;
    }

    if ((err = snd_card_get_name(card, &card_name)) != 0)
    {
        qCWarning(plugin, "snd_card_get_name() failed: %s",
                 snd_strerror(-err));
        card_name = strdup("Unknown soundcard");
    }
    m_ui->mixerCardComboBox->addItem(QString::fromLatin1(card_name));

    snd_pcm_info_alloca(&pcm_info);

    qCDebug(plugin, "detected sound cards:");

    for (;;)
    {
        QString device;
        if ((err = snd_ctl_pcm_next_device(ctl, &pcm_device)) < 0)
        {
            qCWarning(plugin, "snd_ctl_pcm_next_device() failed: %s",
                     snd_strerror(-err));
            pcm_device = -1;
        }
        if (pcm_device < 0)
            break;

        snd_pcm_info_set_device(pcm_info, pcm_device);
        snd_pcm_info_set_subdevice(pcm_info, 0);
        snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);

        if ((err = snd_ctl_pcm_info(ctl, pcm_info)) < 0)
        {
            if (err != -ENOENT)
                qCWarning(plugin, "get_devices_for_card(): "
                         "snd_ctl_pcm_info() "
                         "failed (%d:%d): %s.", card,
                         pcm_device, snd_strerror(-err));
        }
        device = QStringLiteral("hw:%1,%2").arg(card).arg(pcm_device);
        m_devices << device;
        QString str = QStringLiteral("%1: %2 (%3)").arg(QString::fromLatin1(card_name), QString::fromLatin1(snd_pcm_info_get_name(pcm_info)), device);
        qCDebug(plugin) << str;
        m_ui->deviceComboBox->addItem(str);
    }

    free(card_name);
    snd_ctl_close(ctl);
}

void AlsaSettingsDialog::getMixerDevices(QString card)
{
    m_ui->mixerDeviceComboBox->clear();
    snd_mixer_t *mixer;
    snd_mixer_elem_t *current;

    if (getMixer(&mixer, card) < 0)
        return;

    current = snd_mixer_first_elem(mixer);

    while (current)
    {
        const char *sname = snd_mixer_selem_get_name(current);
        if (snd_mixer_selem_is_active(current) &&
                snd_mixer_selem_has_playback_volume(current))
            m_ui->mixerDeviceComboBox->addItem(QString::fromLatin1(sname));
        current = snd_mixer_elem_next(current);
    }
}

void AlsaSettingsDialog::setText(int n)
{
    m_ui->deviceComboBox->setEditText(m_devices.at(n));
}

void AlsaSettingsDialog::accept()
{
    qCDebug(plugin, "AlsaSettingsDialog (ALSA):: writeSettings()");
    QSettings settings;
    settings.beginGroup(u"ALSA"_s);
    settings.setValue(u"device"_s, m_ui->deviceComboBox->currentText ());
    settings.setValue(u"buffer_time"_s, m_ui->bufferSpinBox->value());
    settings.setValue(u"period_time"_s, m_ui->periodSpinBox->value());
    if(m_ui->mixerCardComboBox->currentIndex() >= 0)
    {
        QString card = m_cards.at(m_ui->mixerCardComboBox->currentIndex());
        settings.setValue(u"mixer_card"_s, card);
    }
    settings.setValue(u"mixer_device"_s, m_ui->mixerDeviceComboBox->currentText ());
    settings.setValue(u"use_mmap"_s, m_ui->mmapCheckBox->isChecked());
    settings.setValue(u"use_snd_pcm_pause"_s, m_ui->pauseCheckBox->isChecked());
    settings.endGroup();
    QDialog::accept();
}

int AlsaSettingsDialog::getMixer(snd_mixer_t **mixer, QString card)
{
    int err;

    if ((err = snd_mixer_open(mixer, 0)) < 0)
    {
        qCWarning(plugin, "alsa_get_mixer(): "
                 "Failed to open empty mixer: %s", snd_strerror(-err));
        mixer = nullptr;
        return -1;
    }
    if ((err = snd_mixer_attach(*mixer, card.toLatin1().constData())) < 0)
    {
        qCWarning(plugin, "alsa_get_mixer(): "
                 "Attaching to mixer %s failed: %s", qPrintable(card), snd_strerror(-err));
        return -1;
    }
    if ((err = snd_mixer_selem_register(*mixer, nullptr, nullptr)) < 0)
    {
        qCWarning(plugin, "alsa_get_mixer(): "
                 "Failed to register mixer: %s", snd_strerror(-err));
        return -1;
    }
    if ((err = snd_mixer_load(*mixer)) < 0)
    {
        qCWarning(plugin, "alsa_get_mixer(): Failed to load mixer: %s",
                 snd_strerror(-err));
        return -1;
    }

    return (*mixer != nullptr);
}

void AlsaSettingsDialog::showMixerDevices(int d)
{
    if (0<=d && d<m_cards.size())
        getMixerDevices(m_cards.at(d));
}
