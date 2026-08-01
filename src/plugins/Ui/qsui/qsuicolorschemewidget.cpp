/***************************************************************************
 *   Copyright (C) 2026 by Ilya Kotov                                      *
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

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <qmmpui/colorwidget.h>
#include "qsuicolorscheme.h"
#include "qsuicolorschemewidget.h"

QSUiColorSchemeWidget::QSUiColorSchemeWidget(QWidget *parent) : QWidget(parent)
{
    m_visualizatioColorsGroupBox = new QGroupBox(tr("Visualization Colors"), this);
    m_playlistColorsGroupBox = new QGroupBox(tr("Playlist Colors"), this);
    m_waveformSeekBarGroupBox = new QGroupBox(tr("Waveform Seekbar Colors"), this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    mainLayout->addWidget(m_visualizatioColorsGroupBox);
    mainLayout->addWidget(m_playlistColorsGroupBox);
    mainLayout->addWidget(m_waveformSeekBarGroupBox);
    mainLayout->addSpacerItem(new QSpacerItem(0, 0));

    m_visualizatioColorsGroupBox->setLayout(new QFormLayout);
    m_playlistColorsGroupBox->setLayout(new QFormLayout);
    m_waveformSeekBarGroupBox->setLayout(new QFormLayout);

    const auto titles = QSUiColorScheme::getTitles();
    int labelWidth = 0;

    for(int i = QSUiColorScheme::VIS_COLOR_1; i <= QSUiColorScheme::WFSB_RMS; ++i)
    {
        QGroupBox *parentWidget = nullptr;
        if(i <= QSUiColorScheme::VIS_BACKGROUND)
            parentWidget = m_visualizatioColorsGroupBox;
        else if(i <= QSUiColorScheme::PL_CURRENT_TRACK_BACKGROUND)
            parentWidget = m_playlistColorsGroupBox;
        else
            parentWidget = m_waveformSeekBarGroupBox;

        QFormLayout *layout = static_cast<QFormLayout *>(parentWidget->layout());

        if(i == QSUiColorScheme::PL_BACKGROUND_1)
        {
            m_plSystemColorsCheckBox = new QCheckBox(tr("Use system colors"), parentWidget);
            layout->addRow(m_plSystemColorsCheckBox);
            connect(m_plSystemColorsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
                setColorWidgetsEnabled(QSUiColorScheme::PL_BACKGROUND_1, QSUiColorScheme::PL_HIGHLIGHTED_TEXT, !checked);
            } );
        }
        else if(i == QSUiColorScheme::PL_GROUP_TEXT)
        {
            m_plOverrideGroupColorsCheckBox = new QCheckBox(tr("Override group colors"), parentWidget);
            layout->addRow(m_plOverrideGroupColorsCheckBox);
            connect(m_plOverrideGroupColorsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
                setColorWidgetsEnabled(QSUiColorScheme::PL_GROUP_TEXT, QSUiColorScheme::PL_GROUP_BACKGROUND, checked);
            } );
        }
        else if(i == QSUiColorScheme::PL_CURRENT_TRACK_TEXT)
        {
            m_plOverrideCurrentTrackColorsCheckBox = new QCheckBox(tr("Override current track colors"), parentWidget);
            layout->addRow(m_plOverrideCurrentTrackColorsCheckBox);
            connect(m_plOverrideCurrentTrackColorsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
                setColorWidgetsEnabled(QSUiColorScheme::PL_CURRENT_TRACK_TEXT, QSUiColorScheme::PL_CURRENT_TRACK_BACKGROUND, checked);
            } );
        }

        QSUiColorScheme::QSUiColorRole role = static_cast<QSUiColorScheme::QSUiColorRole>(i);
        QLabel *label = new QLabel(titles.value(role), parentWidget);
        ColorWidget *colorWidget = new ColorWidget(parentWidget);
        if(role == QSUiColorScheme::WFSB_PROGRESSBAR)
            colorWidget->setOptions(QColorDialog::ShowAlphaChannel);
        colorWidget->setColor(m_scheme.color(role));
        colorWidget->setFixedSize(20, 20);
        layout->addRow(label, colorWidget);
        m_colorLabels.insert(role, label);
        m_colorWidges.insert(role, colorWidget);
        labelWidth = qMax(labelWidth, label->sizeHint().width());
    }

    setColorWidgetsEnabled(QSUiColorScheme::PL_GROUP_TEXT, QSUiColorScheme::PL_CURRENT_TRACK_BACKGROUND, false);

    for(QLabel *label : std::as_const(m_colorLabels))
        label->setMinimumWidth(labelWidth);
}

void QSUiColorSchemeWidget::loadDefaults(bool darkMode)
{
    m_scheme.loadDefaults(darkMode);
    for(auto it = m_colorWidges.cbegin(); it != m_colorWidges.cend(); ++it)
        it.value()->setColor(m_scheme.color(it.key()));

    m_plSystemColorsCheckBox->setChecked(m_scheme.plUseSystemColors());
    m_plOverrideGroupColorsCheckBox->setChecked(m_scheme.plOverrideGroupColors());
    m_plOverrideCurrentTrackColorsCheckBox->setChecked(m_scheme.plOverrideCurrentTrackColors());
}

void QSUiColorSchemeWidget::load(const QSettings *settings, bool darkMode)
{
    m_scheme.load(settings, darkMode);
    for(auto it = m_colorWidges.cbegin(); it != m_colorWidges.cend(); ++it)
        it.value()->setColor(m_scheme.color(it.key()));

    m_plSystemColorsCheckBox->setChecked(m_scheme.plUseSystemColors());
    m_plOverrideGroupColorsCheckBox->setChecked(m_scheme.plOverrideGroupColors());
    m_plOverrideCurrentTrackColorsCheckBox->setChecked(m_scheme.plOverrideCurrentTrackColors());
}

void QSUiColorSchemeWidget::write(QSettings *settings, bool darkMode)
{
    for(auto it = m_colorWidges.cbegin(); it != m_colorWidges.cend(); ++it)
        m_scheme.setColor(it.key(), it.value()->color());

    m_scheme.setPlUseSystemColors(m_plSystemColorsCheckBox->isChecked());
    m_scheme.setPlOverrideGroupColors(m_plOverrideGroupColorsCheckBox->isChecked());
    m_scheme.setPlOverrideCurrentTrackColors(m_plOverrideCurrentTrackColorsCheckBox->isChecked());
    m_scheme.write(settings, darkMode);
}

void QSUiColorSchemeWidget::setColorWidgetsEnabled(QSUiColorScheme::QSUiColorRole from, QSUiColorScheme::QSUiColorRole to, bool enabled)
{
    for(int j = from; j <= to; ++j)
    {
        QSUiColorScheme::QSUiColorRole role = static_cast<QSUiColorScheme::QSUiColorRole>(j);
        m_colorLabels.value(role)->setEnabled(enabled);
        m_colorWidges.value(role)->setEnabled(enabled);
    }
}
