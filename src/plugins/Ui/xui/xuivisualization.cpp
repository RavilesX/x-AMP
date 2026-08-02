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
    constexpr int BAR_WIDTH = 4;
    constexpr int BAR_GAP = 2;

    //Maps FFT bins to bars logarithmically: low frequencies carry most of the
    //musical information, so they get proportionally more bars than the highs.
    int binForBar(int bar, int bars)
    {
        const qreal t = qreal(bar) / qreal(bars);
        return int(std::pow(qreal(QMMP_VISUAL_FFT_SIZE), t));
    }

    //Visual hands out samples in the ±32768 range, not normalised, so the raw
    //FFT magnitudes are large. Same curve the bundled analysers use --
    //20 / log(256) over a value already divided by 32768 -- rescaled to 0..1.
    constexpr qreal Y_SCALE = 3.60673760222;
    constexpr qreal STEPS = 15.0;

    qreal magnitude(qreal raw)
    {
        const qreal y = raw / 32768.0;
        if(y <= 1.0)
            return 0.0; //log of anything below 1 is negative: silence
        return qBound(0.0, std::log(y) * Y_SCALE / STEPS, 1.0);
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
            //RMS of typical music sits well below 1.0; lift it so the meter
            //uses its full height without pinning
            m_levels[c] = qMax(qBound(0.0, rms * 3.0, 1.0), m_levels[c] - FALLOFF);
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
