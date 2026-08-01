/***************************************************************************
 *   Copyright (C) 2019-2026 by Ilya Kotov                                 *
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
#include "listenbrainzsettingsdialog.h"
#include "ui_listenbrainzsettingsdialog.h"

ListenBrainzSettingsDialog::ListenBrainzSettingsDialog(QWidget *parent) :
    QDialog(parent), m_ui(new Ui::ListenBrainzSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    m_ui->userTokenLineEdit->setText(settings.value(u"ListenBrainz/user_token"_s).toString());
}

ListenBrainzSettingsDialog::~ListenBrainzSettingsDialog()
{
    delete m_ui;
}

void ListenBrainzSettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(u"ListenBrainz/user_token"_s, m_ui->userTokenLineEdit->text());
    QDialog::accept();
}
