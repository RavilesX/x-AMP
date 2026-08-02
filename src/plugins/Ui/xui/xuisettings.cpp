/***************************************************************************
 *   Copyright (C) 2026 by x-AMP contributors                              *
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

#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include "xuisettings.h"

const QString XUiSettings::ShowEqualizerKey = QStringLiteral("XUi/show_equalizer");
const QString XUiSettings::ShowPlaylistKey  = QStringLiteral("XUi/show_playlist");
const QString XUiSettings::HideOnCloseKey   = QStringLiteral("XUi/hide_on_close");

XUiSettings::XUiSettings(QWidget *parent) : QWidget(parent)
{
    QSettings settings;

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    QLabel *sections = new QLabel(tr("Sections"), this);
    QFont bold = sections->font();
    bold.setBold(true);
    sections->setFont(bold);
    layout->addWidget(sections);

    m_showEqualizer = new QCheckBox(tr("Show equalizer"), this);
    m_showEqualizer->setChecked(settings.value(ShowEqualizerKey, true).toBool());
    layout->addWidget(m_showEqualizer);

    m_showPlaylist = new QCheckBox(tr("Show playlist"), this);
    m_showPlaylist->setChecked(settings.value(ShowPlaylistKey, true).toBool());
    layout->addWidget(m_showPlaylist);

    QLabel *window = new QLabel(tr("Window"), this);
    window->setFont(bold);
    layout->addSpacing(6);
    layout->addWidget(window);

    m_hideOnClose = new QCheckBox(tr("Hide instead of quitting when closed"), this);
    m_hideOnClose->setChecked(settings.value(HideOnCloseKey, false).toBool());
    layout->addWidget(m_hideOnClose);

    layout->addStretch(1);
}

void XUiSettings::writeSettings()
{
    QSettings settings;
    settings.setValue(ShowEqualizerKey, m_showEqualizer->isChecked());
    settings.setValue(ShowPlaylistKey, m_showPlaylist->isChecked());
    settings.setValue(HideOnCloseKey, m_hideOnClose->isChecked());
}
