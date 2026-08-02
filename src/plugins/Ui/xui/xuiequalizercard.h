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

#ifndef XUIEQUALIZERCARD_H
#define XUIEQUALIZERCARD_H

#include <QList>
#include <QStringList>
#include <QWidget>
#include <qmmp/eqsettings.h>

class QLabel;
class SoundCore;
class QmmpSettings;
class XUiEqSlider;
class XUiToggle;
class XUiMenuButton;
class XUiIconButton;

/*!
 * Ten-band equalizer with a preamp, matching the frequencies the interface
 * labels. Presets share ~/.config/xamp/eq.preset with the skinned interface,
 * so a preset saved in one shows up in the other.
 */
class XUiEqualizerCard : public QWidget
{
    Q_OBJECT
public:
    explicit XUiEqualizerCard(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void applySettings();
    void showPresetMenu();
    void reloadFromSettings();
    /*! Applies the preset saved for the track that just started, if any. */
    void applyAutoPreset();

private:
    QWidget *buildHeader();
    QWidget *buildBands();
    void loadPresets();
    void applyPreset(const EqSettings &preset, const QString &name);
    void savePreset();
    void saveAutoPreset();
    void setFlat();
    /*! File name of the current track, which is what auto presets key on. */
    QString currentTrackKey() const;

    QmmpSettings *m_settings;
    SoundCore *m_core;
    XUiToggle *m_enabled;
    XUiToggle *m_auto;
    XUiMenuButton *m_presetButton;
    XUiEqSlider *m_preamp;
    QList<XUiEqSlider *> m_bands;
    QList<EqSettings> m_presets;
    QStringList m_presetNames;
    QList<EqSettings> m_autoPresets;
    QStringList m_autoPresetNames;
};

#endif
