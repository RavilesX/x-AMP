/***************************************************************************
 *   Copyright (C) 2007-2026 by Ilya Kotov                                 *
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
#include <QTimer>
#include <QSettings>
#include <QPainter>
#include <QMenu>
#include <QActionGroup>
#include <QPaintEvent>
#include <math.h>
#include <stdlib.h>
#include <qmmp/buffer.h>
#include <qmmp/output.h>
#include "analyzer.h"

Analyzer::Analyzer(QWidget *parent) : Visual (parent)
{
    setWindowTitle (tr("Qmmp Analyzer"));
    setMinimumSize(2 * 300 - 30, 105);
    m_timer = new QTimer (this);
    connect(m_timer, &QTimer::timeout, this, &Analyzer::timeout);

    clear();
    createMenu();
    readSettings();
}

Analyzer::~Analyzer()
{
    delete [] m_peaks;
    delete [] m_intern_vis_data;
    delete [] m_x_scale;
}

void Analyzer::start()
{
    m_running = true;
    if(isVisible())
        m_timer->start();
}

void Analyzer::stop()
{
    m_running = false;
    m_timer->stop();
    clear();
}

void Analyzer::clear()
{
    m_rows = 0;
    m_cols = 0;
    update();
}

void Analyzer::timeout()
{
    if(takeFFTData(m_left_buffer, m_right_buffer))
    {
        process();
        update();
    }
}

void Analyzer::toggleFullScreen()
{
    setWindowState(windowState() ^Qt::WindowFullScreen);
}

void Analyzer::readSettings()
{
    QSettings settings;
    settings.beginGroup("Analyzer"_L1);
    m_peaks_falloff = settings.value("peak_falloff"_L1, 0.2).toDouble();
    m_analyzer_falloff = settings.value("analyzer_falloff"_L1, 2.2).toDouble();
    m_show_peaks = settings.value("show_peaks"_L1, true).toBool();
    m_timer->setInterval(1000 / settings.value("refresh_rate"_L1, 25).toInt());
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    m_color1 = QColor::fromString(settings.value("color1"_L1, u"Green"_s).toString());
    m_color2 = QColor::fromString(settings.value("color2"_L1, u"Yellow"_s).toString());
    m_color3 = QColor::fromString(settings.value("color3"_L1, u"Red"_s).toString());
    m_bgColor = QColor::fromString(settings.value("bg_color"_L1, u"Black"_s).toString());
    m_peakColor = QColor::fromString(settings.value("peak_color"_L1, u"Cyan"_s).toString());
#else
    m_color1.setNamedColor(settings.value("color1"_L1, u"Green"_s).toString());
    m_color2.setNamedColor(settings.value("color2"_L1, u"Yellow"_s).toString());
    m_color3.setNamedColor(settings.value("color3"_L1, u"Red"_s).toString());
    m_bgColor.setNamedColor(settings.value("bg_color"_L1, u"Black"_s).toString());
    m_peakColor.setNamedColor(settings.value("peak_color"_L1, u"Cyan"_s).toString());
#endif
    m_cell_size = settings.value("cells_size"_L1, QSize(15, 6)).toSize();


    if(!m_update)
    {
        m_update = true;
        m_peaksAction->setChecked(m_show_peaks);

        for(QAction *act : m_fpsGroup->actions ())
        {
            if (m_timer->interval() == 1000 / act->data().toInt())
                act->setChecked(true);
        }
        for(QAction *act : m_peaksFalloffGroup->actions ())
        {
            if (m_peaks_falloff == act->data().toDouble())
                act->setChecked(true);
        }
        for(QAction *act : m_analyzerFalloffGroup->actions ())
        {
            if (m_analyzer_falloff == act->data().toDouble())
                act->setChecked(true);
        }

        //fallback
        if(!m_fpsGroup->checkedAction())
        {
            m_fpsGroup->actions().at(1)->setChecked(true);
            m_timer->setInterval(1000 / 25);
        }
        if(!m_peaksFalloffGroup->checkedAction())
        {
            m_peaksFalloffGroup->actions().at(1)->setChecked(2);
            m_peaks_falloff = 0.2;
        }
        if(!m_peaksFalloffGroup->checkedAction())
        {
            m_peaksFalloffGroup->actions().at(1)->setChecked(2);
            m_analyzer_falloff = 2.2;
        }

        restoreGeometry(settings.value("geometry"_L1).toByteArray());
    }
}

void Analyzer::writeSettings()
{
    QSettings settings;
    settings.beginGroup("Analyzer"_L1);
    QAction *act = m_fpsGroup->checkedAction();
    settings.setValue("refresh_rate"_L1, act ? act->data().toInt() : 25);
    act = m_peaksFalloffGroup->checkedAction();
    settings.setValue("peak_falloff"_L1, act ? act->data().toDouble() : 0.2);
    act = m_analyzerFalloffGroup->checkedAction();
    settings.setValue("analyzer_falloff"_L1, act ? act->data().toDouble() : 2.2);
    settings.setValue("show_peaks"_L1, m_peaksAction->isChecked());
    settings.endGroup();
}

void Analyzer::hideEvent (QHideEvent *)
{
    m_timer->stop();
}

void Analyzer::showEvent (QShowEvent *)
{
    if(m_running)
        m_timer->start();
}

void Analyzer::closeEvent (QCloseEvent *event)
{
    //save geometry
    QSettings settings;
    settings.setValue("Analyzer/geometry"_L1, saveGeometry());
    Visual::closeEvent(event); //removes visualization object
}

void Analyzer::paintEvent (QPaintEvent * e)
{
    QPainter painter (this);
    painter.fillRect(e->rect(),m_bgColor);
    draw(&painter);
}

void Analyzer::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton)
        m_menu->exec(e->globalPosition().toPoint());
}

void Analyzer::process()
{
    int rows = (height() - 2) / m_cell_size.height();
    int cols = (width() - 2) / m_cell_size.width() / 2;

    if(m_rows != rows || m_cols != cols)
    {
        m_rows = rows;
        m_cols = cols;
        delete [] m_peaks;
        delete [] m_intern_vis_data;
        delete [] m_x_scale;
        m_peaks = new double[m_cols * 2] { 0 };
        m_intern_vis_data = new double[m_cols * 2] { 0 };
        m_x_scale = new int[m_cols + 1] { 0 };

        for(int i = 0; i < m_cols + 1; ++i)
        {
            m_x_scale[i] = powf(255.0, float(i) / m_cols);
            if(i > 0)
            {
                if(m_x_scale[i - 1] >= m_x_scale[i]) //avoid several bars in a row with the same frequency
                    m_x_scale[i] = qMin(m_x_scale[i - 1] + 1, m_cols);
            }
        }
    }

    double y_scale = (double) 1.25 * m_rows / log(256);

    for(int i = 0; i < m_cols; i++)
    {
        int j = m_cols * 2 - i - 1; //mirror index
        short yl = 0;
        short yr = 0;
        int magnitude_l = 0;
        int magnitude_r = 0;

        if(m_x_scale[i] == m_x_scale[i + 1])
        {
            yl = static_cast<int>(m_left_buffer[m_x_scale[i]]) >> 15; //32768
            yr = static_cast<int>(m_right_buffer[m_x_scale[i]]) >> 15;
        }
        for(int k = m_x_scale[i]; k < m_x_scale[i + 1]; k++)
        {
            yl = qMax(static_cast<int>(m_left_buffer[k]) >> 15, yl);
            yr = qMax(static_cast<int>(m_right_buffer[k]) >> 15, yr);
        }

        if(yl > 0)
        {
            magnitude_l = qBound(0, int(log(yl) * y_scale), m_rows);
        }
        if(yr > 0)
        {
            magnitude_r = qBound(0, int(log(yr) * y_scale), m_rows);
        }

        m_intern_vis_data[i] -= m_analyzer_falloff * m_rows / 15;
        m_intern_vis_data[i] = magnitude_l > m_intern_vis_data[i] ? magnitude_l : m_intern_vis_data[i];

        m_intern_vis_data[j] -= m_analyzer_falloff * m_rows / 15;
        m_intern_vis_data[j] = magnitude_r > m_intern_vis_data[j] ? magnitude_r : m_intern_vis_data[j];

        if (m_show_peaks)
        {
            m_peaks[i] -= m_peaks_falloff * m_rows / 15;
            m_peaks[i] = magnitude_l > m_peaks[i] ? magnitude_l : m_peaks[i];

            m_peaks[j] -= m_peaks_falloff * m_rows / 15;
            m_peaks[j] = magnitude_r > m_peaks[j] ? magnitude_r : m_peaks[j];
        }
    }
}

void Analyzer::draw(QPainter *p)
{
    QBrush brush(Qt::SolidPattern);
    int rdx = qMax(0, width() - 2 * m_cell_size.width() * m_cols);

    for (int j = 0; j < m_cols * 2; ++j)
    {
        int x = j * m_cell_size.width() + 1;
        if(j >= m_cols)
            x += rdx; //correct right part position

        for (int i = 0; i <= m_intern_vis_data[j]; ++i)
        {
            if (i <= m_rows/3)
                brush.setColor(m_color1);
            else if (i > m_rows/3 && i <= 2 * m_rows / 3)
                brush.setColor(m_color2);
            else
                brush.setColor(m_color3);

            p->fillRect (x, height() - i * m_cell_size.height() + 1,
                         m_cell_size.width() - 2, m_cell_size.height() - 2, brush);
        }

        if (m_show_peaks)
        {
            p->fillRect (x, height() - int(m_peaks[j])*m_cell_size.height() + 1,
                         m_cell_size.width() - 2, m_cell_size.height() - 2, m_peakColor);
        }
    }
}

void Analyzer::createMenu()
{
    m_menu = new QMenu (this);
    connect(m_menu, &QMenu::triggered, this, &Analyzer::writeSettings);
    connect(m_menu, &QMenu::triggered, this, &Analyzer::readSettings);

    m_peaksAction = m_menu->addAction(tr("Peaks"));
    m_peaksAction->setCheckable(true);

    QMenu *refreshRate = m_menu->addMenu(tr("Refresh Rate"));
    m_fpsGroup = new QActionGroup(this);
    m_fpsGroup->setExclusive(true);
    m_fpsGroup->addAction(tr("50 fps"))->setData(50);
    m_fpsGroup->addAction(tr("25 fps"))->setData(25);
    m_fpsGroup->addAction(tr("10 fps"))->setData(10);
    m_fpsGroup->addAction(tr("5 fps"))->setData(5);
    for(QAction *act : m_fpsGroup->actions ())
    {
        act->setCheckable(true);
        refreshRate->addAction(act);
    }

    QMenu *analyzerFalloff = m_menu->addMenu(tr("Analyzer Falloff"));
    m_analyzerFalloffGroup = new QActionGroup(this);
    m_analyzerFalloffGroup->setExclusive(true);
    m_analyzerFalloffGroup->addAction(tr("Slowest"))->setData(1.2);
    m_analyzerFalloffGroup->addAction(tr("Slow"))->setData(1.8);
    m_analyzerFalloffGroup->addAction(tr("Medium"))->setData(2.2);
    m_analyzerFalloffGroup->addAction(tr("Fast"))->setData(2.4);
    m_analyzerFalloffGroup->addAction(tr("Fastest"))->setData(2.8);
    for(QAction *act : m_analyzerFalloffGroup->actions())
    {
        act->setCheckable(true);
        analyzerFalloff->addAction(act);
    }

    QMenu *peaksFalloff = m_menu->addMenu(tr("Peaks Falloff"));
    m_peaksFalloffGroup = new QActionGroup(this);
    m_peaksFalloffGroup->setExclusive(true);
    m_peaksFalloffGroup->addAction(tr("Slowest"))->setData(0.05);
    m_peaksFalloffGroup->addAction(tr("Slow"))->setData(0.1);
    m_peaksFalloffGroup->addAction(tr("Medium"))->setData(0.2);
    m_peaksFalloffGroup->addAction(tr("Fast"))->setData(0.4);
    m_peaksFalloffGroup->addAction(tr("Fastest"))->setData(0.8);
    for(QAction *act : m_peaksFalloffGroup->actions())
    {
        act->setCheckable(true);
        peaksFalloff->addAction(act);
    }
    m_menu->addSeparator();
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
    QAction *fullScreenAction = m_menu->addAction(tr("&Full Screen"), this, &Analyzer::toggleFullScreen, tr("F"));
#else
    QAction *fullScreenAction = m_menu->addAction(tr("&Full Screen"), tr("F"), this, &Analyzer::toggleFullScreen);
#endif
    addAction(fullScreenAction);
    update();
}
