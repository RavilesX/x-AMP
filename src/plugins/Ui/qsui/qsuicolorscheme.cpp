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

#include <QPalette>
#include <QApplication>
#include <qmmp/qmmp.h>
#include "qsuicolorscheme.h"

QSUiColorScheme::QSUiColorScheme()
{
    QString normalBg = qApp->palette().color(QPalette::Base).name();
    QString alternate = qApp->palette().color(QPalette::AlternateBase).name();
    QString selectedBg = qApp->palette().color(QPalette::Highlight).name();
    QString normal = qApp->palette().color(QPalette::Text).name();
    QString current = qApp->palette().color(QPalette::Text).name();
    QString highlighted = qApp->palette().color(QPalette::HighlightedText).name();
    QString groupText = qApp->palette().color(QPalette::Text).name();

    m_descriptors = {
        { VIS_COLOR_1,                 { u"vis_color1"_s, u"#BECBFF"_s, u"#9E9E9E"_s }},
        { VIS_COLOR_2,                 { u"vis_color2"_s, u"#BECBFF"_s, u"#9E9E9E"_s }},
        { VIS_COLOR_3,                 { u"vis_color3"_s, u"#BECBFF"_s, u"#9E9E9E"_s }},
        { VIS_PEAKS,                   { u"vis_peak_color"_s, u"#DDDDDD"_s, u"#DCDCDC"_s }},
        { VIS_BACKGROUND,              { u"vis_bg_color"_s, u"Black"_s, u"Black"_s }},
        { PL_BACKGROUND_1,             { u"pl_bg1_color"_s, normalBg, u"#3D3D3D"_s }},
        { PL_BACKGROUND_2,             { u"pl_bg2_color"_s, alternate, u"#5C5B5A"_s }},
        { PL_HIGHLIGHTED_BACKGROUND,   { u"pl_highlight_color"_s, selectedBg, u"#12608a"_s }},
        { PL_SPLITTER,                 { u"pl_splitter_color"_s, normal, u"#FFFFFF"_s }},
        { PL_NORMAL_TEXT,              { u"pl_normal_text_color"_s, normal, u"#FFFFFF"_s }},
        { PL_HIGHLIGHTED_TEXT,         { u"pl_hl_text_color"_s, highlighted, u"#F9F9F9"_s }},
        { PL_GROUP_TEXT,               { u"pl_group_text"_s, groupText,  u"#F9F9F9"_s }},
        { PL_GROUP_BACKGROUND,         { u"pl_group_bg"_s, normalBg, u"#3D3D3D"_s }},
        { PL_CURRENT_TRACK_TEXT,       { u"pl_current_text_color"_s, current, u"#FFFFFF"_s }},
        { PL_CURRENT_TRACK_BACKGROUND, { u"pl_current_bg_color"_s, normalBg, u"#3D3D3D"_s }},
        { WFSB_BACKGROUND,             { u"wfsb_bg_color"_s, u"Black"_s, u"Black"_s }},
        { WFSB_PROGRESSBAR,            { u"wfsb_progressbar_color"_s, u"#9633CA10"_s, u"#9633CA10"_s }},
        { WFSB_WAVEFORM,               { u"wfsb_waveform_color"_s, u"#BECBFF"_s, u"#9E9E9E"_s }},
        { WFSB_RMS,                    { u"wfsb_rms_color"_s, u"#DDDDDD"_s, u"#DDDDDD"_s }}
    };

    for(auto it = m_descriptors.cbegin(); it != m_descriptors.cend(); ++it)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        m_colors.insert(it.key(), QColor::fromString(it.value().defaultLightColor));
#else
        QColor color;
        color.setNamedColor(it.value().defaultLightColor);
        m_colors.insert(it.key(), color);
#endif
    }
}

void QSUiColorScheme::loadDefaults(bool darkMode)
{
    for(auto it = m_descriptors.cbegin(); it != m_descriptors.cend(); ++it)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        m_colors.insert(it.key(), QColor::fromString(darkMode ? it.value().defaultDarkColor : it.value().defaultLightColor));
#else
        QColor color;
        color.setNamedColor(darkMode ? it.value().defaultDarkColor : it.value().defaultLightColor);
        m_colors.insert(it.key(), color);
#endif
    }

    m_plSystemColors = true;
    m_plOverrideGroupColors = false;
    m_plOverrideCurrentTrackColors = false;
}

void QSUiColorScheme::load(const QSettings *settings, bool darkMode)
{
    QString suffix = darkMode ? u"_dark"_s : QString();

    for(auto it = m_descriptors.cbegin(); it != m_descriptors.cend(); ++it)
    {
        QString defaultColor = darkMode ? it.value().defaultDarkColor : it.value().defaultLightColor;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        m_colors.insert(it.key(), QColor::fromString(settings->value(it.value().name + suffix, defaultColor).toString()));
#else
        QColor color;
        color.setNamedColor(settings->value(it.value().name + suffix, defaultColor).toString());
        m_colors.insert(it.key(), color);
#endif
    }

    m_plSystemColors = settings->value(u"pl_system_colors"_s + suffix, true).toBool();
    m_plOverrideGroupColors = settings->value(u"pl_override_group_colors"_s + suffix, false).toBool();
    m_plOverrideCurrentTrackColors = settings->value(u"pl_override_current_track_colors"_s + suffix, false).toBool();
}

void QSUiColorScheme::write(QSettings *settings, bool darkMode) const
{
    QString suffix = darkMode ? u"_dark"_s : QString();

    for(int i = VIS_COLOR_1; i <= WFSB_RMS; ++i)
    {
        QSUiColorRole role = static_cast<QSUiColorRole>(i);
        QColor::NameFormat format = i == WFSB_PROGRESSBAR ? QColor::HexArgb : QColor::HexRgb;
        settings->setValue(m_descriptors.value(role).name + suffix, m_colors.value(role).name(format));
    }

    settings->setValue(u"pl_system_colors"_s + suffix, m_plSystemColors);
    settings->setValue(u"pl_override_group_colors"_s + suffix, m_plOverrideGroupColors);
    settings->setValue(u"pl_override_current_track_colors"_s + suffix, m_plOverrideCurrentTrackColors);
}

void QSUiColorScheme::setColor(QSUiColorRole role, const QColor &color)
{
    m_colors.insert(role, color);
}

QColor QSUiColorScheme::color(QSUiColorRole role) const
{
    return m_colors.value(role);
}

void QSUiColorScheme::setPlUseSystemColors(bool plSystemColors)
{
    m_plSystemColors = plSystemColors;
}

void QSUiColorScheme::setPlOverrideGroupColors(bool plOverrideGroupColors)
{
    m_plOverrideGroupColors = plOverrideGroupColors;
}

void QSUiColorScheme::setPlOverrideCurrentTrackColors(bool plOverrideCurrentTrackColors)
{
    m_plOverrideCurrentTrackColors = plOverrideCurrentTrackColors;
}

bool QSUiColorScheme::plUseSystemColors() const
{
    return m_plSystemColors;
}

bool QSUiColorScheme::plOverrideGroupColors() const
{
    return m_plOverrideGroupColors;
}

bool QSUiColorScheme::plOverrideCurrentTrackColors() const
{
    return m_plOverrideCurrentTrackColors;
}

QHash<QSUiColorScheme::QSUiColorRole, QString> QSUiColorScheme::getTitles()
{
    QHash<QSUiColorScheme::QSUiColorRole, QString> titles = {
        { VIS_COLOR_1, tr("Color #1:") },
        { VIS_COLOR_2, tr("Color #2:") },
        { VIS_COLOR_3, tr("Color #3:") },
        { VIS_PEAKS, tr("Peaks:") },
        { VIS_BACKGROUND, tr("Background:") },
        { PL_BACKGROUND_1, tr("Background #1:") },
        { PL_BACKGROUND_2, tr("Background #2:") },
        { PL_HIGHLIGHTED_BACKGROUND, tr("Highlighted background:") },
        { PL_SPLITTER, tr("Splitter:") },
        { PL_NORMAL_TEXT, tr("Normal text:") },
        { PL_HIGHLIGHTED_TEXT, tr("Highlighted text:") },
        { PL_GROUP_TEXT, tr("Group text:") },
        { PL_GROUP_BACKGROUND, tr("Group background:") },
        { PL_CURRENT_TRACK_TEXT, tr("Current track text:") },
        { PL_CURRENT_TRACK_BACKGROUND, tr("Current track background:") },
        { WFSB_BACKGROUND, tr("Background:") },
        { WFSB_PROGRESSBAR, tr("Progress bar:") },
        { WFSB_WAVEFORM, tr("Waveform:") },
        { WFSB_RMS, tr("RMS:") }
    };

    return titles;
}
