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

#ifndef QSUICOLORSCHEME_H
#define QSUICOLORSCHEME_H

#include <QCoreApplication>
#include <QHash>
#include <QSettings>
#include <QColor>

class QSUiColorScheme
{
    Q_DECLARE_TR_FUNCTIONS(QSUiColorScheme)
public:
    QSUiColorScheme();

    enum QSUiColorRole
    {
        VIS_COLOR_1 = 0,
        VIS_COLOR_2,
        VIS_COLOR_3,
        VIS_PEAKS,
        VIS_BACKGROUND,
        PL_BACKGROUND_1,
        PL_BACKGROUND_2,
        PL_HIGHLIGHTED_BACKGROUND,
        PL_SPLITTER,
        PL_NORMAL_TEXT,
        PL_HIGHLIGHTED_TEXT,
        PL_GROUP_TEXT,
        PL_GROUP_BACKGROUND,
        PL_CURRENT_TRACK_TEXT,
        PL_CURRENT_TRACK_BACKGROUND,
        WFSB_BACKGROUND,
        WFSB_PROGRESSBAR,
        WFSB_WAVEFORM,
        WFSB_RMS
    };

    void loadDefaults(bool darkMode);
    void load(const QSettings *settings, bool darkMode);
    void write(QSettings *settings, bool darkMode) const;

    void setColor(QSUiColorRole role, const QColor &color);
    QColor color(QSUiColorRole role) const;

    void setPlUseSystemColors(bool plSystemColors);
    void setPlOverrideGroupColors(bool plOverrideGroupColors);
    void setPlOverrideCurrentTrackColors(bool plOverrideCurrentTrackColors);

    bool plUseSystemColors() const;
    bool plOverrideGroupColors() const;
    bool plOverrideCurrentTrackColors() const;

    static QHash<QSUiColorRole, QString> getTitles();

private:
    struct ColorDesc
    {
        QString name;
        QString defaultLightColor;
        QString defaultDarkColor;
    };

    QHash<QSUiColorRole, ColorDesc> m_descriptors;
    QHash<QSUiColorRole, QColor> m_colors;
    bool m_plSystemColors = true;
    bool m_plOverrideGroupColors = false;
    bool m_plOverrideCurrentTrackColors = false;

};

#endif // QSUICOLORSCHEME_H
