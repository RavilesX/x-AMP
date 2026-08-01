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

#ifndef QSUICOLORSCHEMEWIDGET_H
#define QSUICOLORSCHEMEWIDGET_H

#include <QWidget>
#include "qsuicolorscheme.h"

class QGroupBox;
class QLabel;
class QCheckBox;
class ColorWidget;

class QSUiColorSchemeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QSUiColorSchemeWidget(QWidget *parent = nullptr);

    void loadDefaults(bool darkMode);
    void load(const QSettings *settings, bool darkMode);
    void write(QSettings *settings, bool darkMode);

private:
    void setColorWidgetsEnabled(QSUiColorScheme::QSUiColorRole from, QSUiColorScheme::QSUiColorRole to, bool enabled);

    QGroupBox *m_visualizatioColorsGroupBox;
    QGroupBox *m_playlistColorsGroupBox;
    QGroupBox *m_waveformSeekBarGroupBox;
    QSUiColorScheme m_scheme;
    QHash<QSUiColorScheme::QSUiColorRole, QLabel *> m_colorLabels;
    QHash<QSUiColorScheme::QSUiColorRole, ColorWidget *> m_colorWidges;
    QCheckBox *m_plSystemColorsCheckBox;
    QCheckBox *m_plOverrideGroupColorsCheckBox;
    QCheckBox *m_plOverrideCurrentTrackColorsCheckBox;
};

#endif // QSUICOLORSCHEMEWIDGET_H
