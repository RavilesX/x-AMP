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
#ifndef SKINNEDDISPLAY_H
#define SKINNEDDISPLAY_H

#include <QPixmap>
#include <qmmp/statehandler.h>
#include <qmmp/audioparameters.h>
#include "pixmapwidget.h"

class QTimer;
class SkinnedTimeIndicator;
class SkinnedTimeIndicatorModel;
class SkinnedPositionBar;
class SkinnedToggleButton;
class SkinnedTitleBar;
class NumberDisplay;
class SymbolDisplay;
class MonoStereo;
class SkinnedPlayStatus;
class SkinnedVolumeBar;
class SkinnedBalanceBar;
class SkinnedMainWindow;
class SoundCore;
class SkinnedButton;
class SkinnedTextScroller;
class SkinnedVisualization;
class SkinnedTitleBar;
class SkinnedEqWidget;
class SkinnedPlayList;

/**
   @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedDisplay : public PixmapWidget
{
    Q_OBJECT
public:
    SkinnedDisplay(SkinnedMainWindow *parent = nullptr);

    ~SkinnedDisplay();

    void setEQ(SkinnedEqWidget *w);
    void setPL(SkinnedPlayList *w);
    bool isEqualizerVisible() const;
    bool isPlaylistVisible() const;
    bool isRepeatable() const;
    bool isShuffle() const;
    void setIsRepeatable(bool);
    void setIsShuffle(bool);
    void setMinimalMode(bool b = true);
    void setActive(bool b);

public slots:
    void setDuration(qint64);

signals:
    void repeatableToggled(bool);
    void shuffleToggled(bool);

private slots:
    void updateSkin() override;
    void displayVolume();
    void showPosition();
    void updatePosition();
    void setTime(qint64);
    void setState(Qmmp::State state);
    void onAudioPatametersChanged(const AudioParameters &p);

private:
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent*) override;
    void updatePositions();
    void updateMask();

    SkinnedEqWidget *m_equlizer;
    SkinnedPlayList *m_playlist;
    bool m_shaded = false;
    SkinnedPositionBar *m_posbar;
    SkinnedButton *m_previous;
    SkinnedButton *m_play;
    SkinnedButton *m_pause;
    SkinnedButton *m_stop;
    SkinnedButton *m_next;
    SkinnedButton *m_eject;
    SkinnedTextScroller *m_text;
    SkinnedToggleButton *m_eqButton;
    SkinnedToggleButton *m_plButton;
    SkinnedToggleButton *m_shuffleButton;
    SkinnedToggleButton *m_repeatButton;
    SymbolDisplay* m_kbps;
    SymbolDisplay* m_freq;
    MonoStereo* m_monoster;
    SkinnedPlayStatus* m_playstatus;
    SkinnedVolumeBar* m_volumeBar;
    SkinnedBalanceBar* m_balanceBar;
    SkinnedMainWindow* m_mw;
    SkinnedVisualization* m_vis;
    SkinnedTimeIndicatorModel* m_timeIndicatorModel;
    SkinnedTimeIndicator* m_timeIndicator;
    SkinnedTitleBar *m_titlebar;
    SoundCore *m_core;
    QWidget *m_aboutWidget;
    QTimer *m_volumeDisplayTimer;
};

#endif
