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
#ifndef SKINNEDVISUALIZATION_H
#define SKINNEDVISUALIZATION_H

#include <QWidget>
#include <QResizeEvent>
#include <qmmp/visual.h>

class QTimer;
class QMenu;
class QActionGroup;

class SkinnedVisualBase
{
public:
    virtual ~SkinnedVisualBase() = default;
    virtual void clear() = 0;
    virtual bool process(float *l) = 0;
    virtual void draw(QPainter *) = 0;
    virtual QString name() = 0;
    virtual bool useFFT() const = 0;
};

class Skin;

class SkinnedVisualization : public Visual
{
    Q_OBJECT

public:
    SkinnedVisualization(QWidget *parent = nullptr);
    virtual ~SkinnedVisualization();

    static SkinnedVisualization *instance();
    void setVisual(SkinnedVisualBase *newvis);

    void paintEvent(QPaintEvent *) override;

protected:
    virtual void hideEvent(QHideEvent *) override;
    virtual void showEvent(QShowEvent *) override;
    virtual void mousePressEvent(QMouseEvent *) override;

public slots:
    void start() override;
    void stop() override;

private slots:
    void timeout();
    void readSettings();
    void writeSettings();

private:
    void clear();
    void drawBackGround();
    void createMenu();

    static SkinnedVisualization *m_instance;
    SkinnedVisualBase *m_vis = nullptr;
    QPixmap m_pixmap;
    QPixmap m_bg;
    QTimer *m_timer;
    Skin *m_skin;
    //menu and actions
    QMenu *m_menu;
    //action groups
    QActionGroup *m_visModeGroup;
    QActionGroup *m_fpsGroup;
    QActionGroup *m_peaksFalloffGroup;
    QActionGroup *m_analyzerFalloffGroup;
    QActionGroup *m_analyzerModeGroup;
    QActionGroup *m_analyzerTypeGroup;
    QAction *m_peaksAction;
    QAction *m_transparentAction;
    int m_ratio;
    float m_buffer[QMMP_VISUAL_NODE_SIZE];
    bool m_update = false;
    bool m_running = false;
};

class SkinnedAnalyzer : public SkinnedVisualBase
{
public:
    SkinnedAnalyzer();
    virtual ~SkinnedAnalyzer() = default;

    void clear() override;
    bool process(float *l) override;
    void draw(QPainter *p) override;
    QString name() override;
    bool useFFT() const override;

private:
    QSize m_size;
    double m_intern_vis_data[75];
    double m_peaks[75];
    double m_peaks_falloff;
    double m_analyzer_falloff;
    bool m_show_peaks;
    bool m_lines;
    int m_mode;
    Skin *m_skin;
};

class SkinnedScope : public SkinnedVisualBase
{
public:
    SkinnedScope();
    virtual ~SkinnedScope() = default;
    void clear() override;
    bool process(float *l) override;
    void draw(QPainter *p) override;
    QString name() override;
    bool useFFT() const override;

private:
    int m_intern_vis_data[76];
    Skin *m_skin;
    int m_ratio;
};

#endif
