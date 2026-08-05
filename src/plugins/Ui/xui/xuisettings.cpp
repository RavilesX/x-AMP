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
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include "xuitheme.h"
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

    //accent colour: the one colour of the interface that is not a fixed
    //neutral, so it is the only one worth offering
    QLabel *colour = new QLabel(tr("Colour"), this);
    colour->setFont(bold);
    layout->addSpacing(6);
    layout->addWidget(colour);

    m_accent = settings.value(XUi::AccentKey, XUi::defaultAccent()).value<QColor>();

    QHBoxLayout *accentRow = new QHBoxLayout;
    accentRow->setSpacing(8);
    accentRow->addWidget(new QLabel(tr("Accent colour:"), this));
    m_accentButton = new QPushButton(this);
    m_accentButton->setFixedSize(64, 26);
    m_accentButton->setCursor(Qt::PointingHandCursor);
    connect(m_accentButton, &QPushButton::clicked, this, &XUiSettings::pickAccent);
    accentRow->addWidget(m_accentButton);
    QPushButton *reset = new QPushButton(tr("Reset"), this);
    connect(reset, &QPushButton::clicked, this, &XUiSettings::resetAccent);
    accentRow->addWidget(reset);
    accentRow->addStretch(1);
    layout->addLayout(accentRow);
    showAccent();

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
    settings.setValue(XUi::AccentKey, m_accent);
}

void XUiSettings::pickAccent()
{
    const QColor picked = QColorDialog::getColor(m_accent, this,
                                                 tr("Accent colour"));
    if(!picked.isValid())
        return; //cancelled
    m_accent = picked;
    showAccent();
}

void XUiSettings::resetAccent()
{
    m_accent = XUi::defaultAccent();
    showAccent();
}

void XUiSettings::showAccent()
{
    //a plain swatch: the sheet styles QPushButton, so the colour has to be
    //stated here to win over it
    m_accentButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; border: 1px solid %2; border-radius: 6px; }")
        .arg(m_accent.name(), XUi::Border.name()));
    m_accentButton->setToolTip(m_accent.name().toUpper());
}
