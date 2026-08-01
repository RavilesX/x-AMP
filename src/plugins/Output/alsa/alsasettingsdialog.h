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
#ifndef ALSASETTINGSDIALOG_H
#define ALSASETTINGSDIALOG_H

#include <QDialog>
extern "C"{
#include <alsa/asoundlib.h> 
}

namespace Ui {
class AlsaSettingsDialog;
}

/**
	@author Ilya Kotov <forkotov02@ya.ru>
*/
class AlsaSettingsDialog : public QDialog
{
Q_OBJECT
public:
    explicit AlsaSettingsDialog(QWidget *parent = nullptr);

    ~AlsaSettingsDialog();

public slots:
    void accept() override;

private slots:
    void setText(int);
    void showMixerDevices(int);

private:
    Ui::AlsaSettingsDialog *m_ui;
    void getCards();
    void getSoftDevices();
    void getCardDevices(int card);
    void getMixerDevices(QString card);
    int getMixer(snd_mixer_t **mixer, QString card);
    QStringList m_devices;
    QStringList m_cards;

};

#endif
