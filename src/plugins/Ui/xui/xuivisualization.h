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

#ifndef XUIVISUALIZATION_H
#define XUIVISUALIZATION_H

#include <QVector>
#include <qmmp/visual.h>

class QTimer;

/*!
 * Spectrum analyser drawn as rounded bars with a vertical gradient.
 *
 * Registered with Visual::add() so the engine feeds it PCM; the widget pulls
 * FFT data on a timer rather than being pushed to, which is the pattern the
 * other bundled interfaces use.
 */
class XUiSpectrum : public Visual
{
    Q_OBJECT
public:
    explicit XUiSpectrum(QWidget *parent = nullptr);
    ~XUiSpectrum();

    void clear();

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;

private slots:
    void timeout();

private:
    void rebuildBands();

    QTimer *m_timer;
    float m_buffer[QMMP_VISUAL_NODE_SIZE] = { 0 };
    QVector<qreal> m_bands;  //current bar heights, 0..1
    QVector<qreal> m_peaks;  //slowly falling peak markers, 0..1
};

/*!
 * The pair of stereo level meters: a grid of small cells lit from the bottom,
 * one column per channel.
 */
class XUiVuMeter : public Visual
{
    Q_OBJECT
public:
    explicit XUiVuMeter(QWidget *parent = nullptr);
    ~XUiVuMeter();

    void clear();

protected:
    void paintEvent(QPaintEvent *) override;
    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;

private slots:
    void timeout();

private:
    QTimer *m_timer;
    float m_left[QMMP_VISUAL_NODE_SIZE] = { 0 };
    float m_right[QMMP_VISUAL_NODE_SIZE] = { 0 };
    qreal m_levels[2] = { 0.0, 0.0 };
};

#endif
