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

#include <QPainter>
#include <QTimer>
#include <cmath>
#include "xuitheme.h"
#include "xuivisualization.h"

namespace
{
    constexpr int REFRESH_MS = 40;      //25 fps, enough to look fluid
    constexpr qreal FALLOFF = 0.055;    //bar decay per frame
    constexpr qreal PEAK_FALLOFF = 0.012;
    constexpr qreal PEAK_HEIGHT = 2.0; //thickness of the falling cap
    constexpr int BAR_WIDTH = 4;
    constexpr int BAR_GAP = 2;

    //Maps FFT bins to bars logarithmically: low frequencies carry most of the
    //musical information, so they get proportionally more bars than the highs.
    int binForBar(int bar, int bars)
    {
        const qreal t = qreal(bar) / qreal(bars);
        return int(std::pow(qreal(QMMP_VISUAL_FFT_SIZE), t));
    }

    //takeFFTData() returns sqrt of the FFT output, whose range fft.c documents
    //as ((FFT_BUFFER_SIZE / 2) * 32768)^2 -- so the magnitudes run up to
    //256 * 32768. Normalise against that and read the result in decibels:
    //a plain log floor cuts off the treble bins, which carry far less energy
    //than the bass, and leaves the right half of the analyser dead.
    constexpr qreal FFT_MAX = 256.0 * 32768.0;
    constexpr qreal FLOOR_DB = -70.0;

    qreal magnitude(qreal raw)
    {
        if(raw <= 0.0)
            return 0.0;
        const qreal db = 20.0 * std::log10(raw / FFT_MAX);
        return qBound(0.0, 1.0 - db / FLOOR_DB, 1.0);
    }
}

// ------------------------------------------------------------------ spectrum

XUiSpectrum::XUiSpectrum(QWidget *parent) : Visual(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    m_timer = new QTimer(this);
    m_timer->setInterval(REFRESH_MS);
    connect(m_timer, &QTimer::timeout, this, &XUiSpectrum::timeout);
    m_timer->start();
}

XUiSpectrum::~XUiSpectrum() = default;

void XUiSpectrum::clear()
{
    m_bands.fill(0.0);
    m_peaks.fill(0.0);
    update();
}

void XUiSpectrum::rebuildBands()
{
    const int count = qMax(1, width() / (BAR_WIDTH + BAR_GAP));
    if(m_bands.size() == count)
        return;
    m_bands = QVector<qreal>(count, 0.0);
    m_peaks = QVector<qreal>(count, 0.0);
}

void XUiSpectrum::resizeEvent(QResizeEvent *)
{
    rebuildBands();
}

void XUiSpectrum::showEvent(QShowEvent *)
{
    m_timer->start();
}

void XUiSpectrum::hideEvent(QHideEvent *)
{
    m_timer->stop(); //no point burning frames on a hidden widget
}

void XUiSpectrum::timeout()
{
    rebuildBands();
    if(m_bands.isEmpty())
        return;

    if(takeFFTData(m_buffer))
    {
        for(int i = 0; i < m_bands.size(); ++i)
        {
            const int from = binForBar(i, m_bands.size());
            const int to = qMax(from + 1, binForBar(i + 1, m_bands.size()));
            qreal peak = 0.0;
            for(int bin = from; bin < to && bin < QMMP_VISUAL_FFT_SIZE; ++bin)
                peak = qMax(peak, qreal(m_buffer[bin]));

            const qreal value = magnitude(peak);
            m_bands[i] = qMax(value, m_bands[i] - FALLOFF);
            m_peaks[i] = qMax(m_bands[i], m_peaks[i] - PEAK_FALLOFF);
        }
    }
    else
    {
        //nothing playing: let everything settle to the floor
        bool moving = false;
        for(int i = 0; i < m_bands.size(); ++i)
        {
            m_bands[i] = qMax(0.0, m_bands[i] - FALLOFF);
            m_peaks[i] = qMax(0.0, m_peaks[i] - PEAK_FALLOFF);
            moving = moving || m_bands[i] > 0.0 || m_peaks[i] > 0.0;
        }
        if(!moving)
            return;
    }
    update();
}

void XUiSpectrum::paintEvent(QPaintEvent *)
{
    if(m_bands.isEmpty())
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);

    const qreal h = height();
    QLinearGradient g(0, h, 0, 0);
    g.setColorAt(0.0, XUi::AccentDeep);
    g.setColorAt(0.6, XUi::Accent);
    g.setColorAt(1.0, XUi::AccentBright);
    const QBrush brush(g);

    for(int i = 0; i < m_bands.size(); ++i)
    {
        const qreal x = i * (BAR_WIDTH + BAR_GAP);
        const qreal barHeight = qMax(2.0, m_bands[i] * h);
        p.setBrush(brush);
        p.drawRoundedRect(QRectF(x, h - barHeight, BAR_WIDTH, barHeight), 1.5, 1.5);

        //The cap that hangs at each band's recent maximum and sinks back
        //towards the bar, as the classic analyser does. Drawn only once it
        //has separated from the bar, so it reads as a marker rather than as
        //a lid the bars always wear.
        const qreal peakHeight = m_peaks[i] * h;
        if(peakHeight > barHeight + PEAK_HEIGHT)
        {
            p.setBrush(XUi::AccentBright);
            p.drawRect(QRectF(x, h - peakHeight - PEAK_HEIGHT, BAR_WIDTH, PEAK_HEIGHT));
        }
    }
}

// ------------------------------------------------------------------ VU meter

XUiVuMeter::XUiVuMeter(QWidget *parent) : Visual(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    m_timer = new QTimer(this);
    m_timer->setInterval(REFRESH_MS);
    connect(m_timer, &QTimer::timeout, this, &XUiVuMeter::timeout);
    m_timer->start();
}

XUiVuMeter::~XUiVuMeter() = default;

void XUiVuMeter::clear()
{
    m_levels[0] = m_levels[1] = 0.0;
    update();
}

void XUiVuMeter::showEvent(QShowEvent *)
{
    m_timer->start();
}

void XUiVuMeter::hideEvent(QHideEvent *)
{
    m_timer->stop();
}

void XUiVuMeter::timeout()
{
    if(takeData(m_left, m_right))
    {
        const float *channels[2] = { m_left, m_right };
        for(int c = 0; c < 2; ++c)
        {
            //RMS reads closer to perceived loudness than a raw peak does.
            //Unlike the FFT output, takeData() hands out PCM already
            //normalised to ±1, so there is nothing to divide here.
            qreal sum = 0.0;
            for(int i = 0; i < QMMP_VISUAL_NODE_SIZE; ++i)
            {
                const qreal sample = qreal(channels[c][i]);
                sum += sample * sample;
            }
            const qreal rms = std::sqrt(sum / QMMP_VISUAL_NODE_SIZE);
            //also read in decibels: music sits around -20 dBFS, so a linear
            //scale would leave the meter in its bottom couple of cells
            qreal level = 0.0;
            if(rms > 0.0)
                level = qBound(0.0, 1.0 - (20.0 * std::log10(rms)) / -45.0, 1.0);
            m_levels[c] = qMax(level, m_levels[c] - FALLOFF);
        }
    }
    else
    {
        if(m_levels[0] <= 0.0 && m_levels[1] <= 0.0)
            return;
        m_levels[0] = qMax(0.0, m_levels[0] - FALLOFF);
        m_levels[1] = qMax(0.0, m_levels[1] - FALLOFF);
    }
    update();
}

void XUiVuMeter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setPen(Qt::NoPen);

    constexpr int ROWS = 12;
    constexpr int CELL_GAP = 2;
    const int columnWidth = (width() - CELL_GAP) / 2;
    const qreal cellHeight = qreal(height() - (ROWS - 1) * CELL_GAP) / ROWS;

    for(int c = 0; c < 2; ++c)
    {
        const int lit = int(std::round(m_levels[c] * ROWS));
        for(int row = 0; row < ROWS; ++row)
        {
            const qreal y = row * (cellHeight + CELL_GAP);
            //row 0 is the top of the widget, so invert to light from below
            const bool on = (ROWS - row) <= lit;
            if(on)
            {
                //warmer towards the top of the meter
                const qreal t = qreal(ROWS - row) / ROWS;
                p.setBrush(t > 0.85 ? XUi::AccentBright : XUi::Accent);
            }
            else
            {
                p.setBrush(XUi::Border);
            }
            p.drawRect(QRectF(c * (columnWidth + CELL_GAP), y, columnWidth, cellHeight));
        }
    }
}
