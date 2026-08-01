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
#ifndef SKINNEDEQWIDGET_H
#define SKINNEDEQWIDGET_H

#include "pixmapwidget.h"
#include <qmmp/output.h>
#include <qmmp/eqsettings.h>

/**
   @author Ilya Kotov <forkotov02@ya.ru>
*/

class QMenu;
class SkinnedEqTitleBar;
class SkinnedEqSlider;
class SkinnedToggleButton;
class SkinnedEqGraph;
class SkinnedButton;
class PlayListTrack;
class SoundCore;

class SkinnedEqWidget : public PixmapWidget
{
    Q_OBJECT
public:
    explicit SkinnedEqWidget(QWidget *parent = nullptr);

    ~SkinnedEqWidget() = default;

    /*!
    * necessary for auto-load presets
    */
    void loadPreset(const QString &name);
    void setMimimalMode(bool b = true);

    void writeSettings();

signals:
    void closed();

private slots:
    void updateSkin() override;
    void readEq();
    void writeEq();
    void showPresetsMenu();
    void reset();
    void showEditor();
    void savePreset();
    void saveAutoPreset();
    void setPresetByName(const QString &name, bool autoPreset = true);
    void removePresetByName(const QString &name, bool autoPreset = true);
    void importWinampEQF();

private:
    void updatePositions();
    void readSettings();
    void createActions();
    void updateMask();
    void setPreset(int index, bool autoPreset = true);
    void setPreset(const EqSettings &preset);

    //events
    void keyPressEvent (QKeyEvent *) override;
    void changeEvent(QEvent*) override;
    void closeEvent(QCloseEvent*) override;
#ifdef QMMP_WS_X11
    bool event(QEvent *event) override;
#endif

    SkinnedEqTitleBar *m_titleBar;
    SkinnedEqSlider *m_preamp;
    SkinnedButton *m_presetButton;
    QList<SkinnedEqSlider*> m_sliders;
    SkinnedToggleButton *m_on;
    SkinnedToggleButton *m_autoButton;
    SkinnedEqGraph *m_eqg;
    QMenu *m_presetsMenu;
    QList<EqSettings> m_presets, m_autoPresets;
    QStringList m_presetNames, m_autoPresetNames;
    bool m_shaded = false;
};

#endif
