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

#ifndef XUIPLAYERCARD_H
#define XUIPLAYERCARD_H

#include <QWidget>
#include <qmmp/qmmp.h>

class QLabel;
class SoundCore;
class MediaPlayer;
class QmmpUiSettings;
class XUiIconButton;
class XUiPlayButton;
class XUiSlider;
class XUiChip;
class XUiSpectrum;
class XUiVuMeter;
class XUiCoverArt;

/*!
 * The top card: artwork, track details, format badges, spectrum, level
 * meters, seek bar and the transport row.
 *
 * Owns no playback state of its own -- it reflects SoundCore and drives
 * MediaPlayer, so it stays correct when playback is changed from elsewhere
 * (MPRIS, the command line, global hotkeys).
 */
class XUiPlayerCard : public QWidget
{
    Q_OBJECT
public:
    explicit XUiPlayerCard(QWidget *parent = nullptr);

    /*!
     * Re-applies the accent to the labels that hold it in their palette.
     *
     * Those are set once, when the label is built, and a repaint does not
     * recompute them -- so without this the artist name and the active
     * MONO/STEREO word keep whatever colour was current at construction.
     */
    void applyAccent();

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void updateTrackInfo();
    void updateState(Qmmp::State state);
    void updateElapsed(qint64 elapsed);
    void updateVolume(int volume);
    void updateBitrate(int bitrate);
    void seek(qint64 position);
    /*! Cycles off -> repeat list -> repeat track. */
    void cycleRepeat();
    /*! Next, wrapping to the first track at the end of the playlist. */
    void nextTrack();
    void updateRepeat();

private:
    QWidget *buildDetails();
    QWidget *buildTransport();
    void updateTimeLabel(qint64 elapsed);

    SoundCore *m_core;
    MediaPlayer *m_player;
    QmmpUiSettings *m_settings;

    XUiCoverArt *m_cover;
    QLabel *m_title;
    QLabel *m_artist;
    QLabel *m_time;
    QLabel *m_mono;
    QLabel *m_stereo;
    XUiChip *m_format;
    XUiChip *m_bitrate;
    XUiChip *m_sampleRate;
    XUiSpectrum *m_spectrum;
    XUiVuMeter *m_vu;
    XUiSlider *m_seek;
    XUiSlider *m_volume;
    XUiPlayButton *m_play;
    XUiIconButton *m_shuffle;
    XUiIconButton *m_repeat;
    XUiIconButton *m_volumeIcon;
};

#endif
