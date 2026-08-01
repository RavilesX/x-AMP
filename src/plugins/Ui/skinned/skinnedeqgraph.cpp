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
#include <QPainter>

#include "skin.h"
#include "skinnedeqgraph.h"

SkinnedEqGraph::SkinnedEqGraph(QWidget *parent) : PixmapWidget (parent)
{
    setPixmap(skin()->getEqPart (Skin::EQ_GRAPH));
    clear();
    m_ratio = skin()->ratio();
    draw();;
    setVisible(!skin()->getEqPart(Skin::EQ_GRAPH).isNull());
}

SkinnedEqGraph::~SkinnedEqGraph()
{}

void SkinnedEqGraph::addValue(int value)
{
    if(m_values.size() >= 10)
        return;
    m_values.append(value);
    if(m_values.size() == 10)
    {
        draw();
    }
}

void SkinnedEqGraph::clear ()
{
    m_values.clear();
    update();
}

void SkinnedEqGraph::init_spline(double * x, double * y, int n, double * y2)
{
    double qn, un;
    double *u = new double[n];

    y2[0] = u[0] = 0.0;

    for(int i = 1; i < n - 1; i++)
    {
        double sig = ((double) x[i] - x[i - 1]) / ((double) x[i + 1] - x[i - 1]);
        double p = sig * y2[i - 1] + 2.0;
        y2[i] = (sig - 1.0) / p;
        u[i] =
            (((double) y[i + 1] - y[i]) / (x[i + 1] - x[i])) -
            (((double) y[i] - y[i - 1]) / (x[i] - x[i - 1]));
        u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
    }
    qn = un = 0.0;

    y2[n - 1] = (un - qn * u[n - 2]) / (qn * y2[n - 2] + 1.0);
    for(int k = n - 2; k >= 0; k--)
        y2[k] = y2[k] * y2[k + 1] + u[k];
    delete[] u;
}

double SkinnedEqGraph::eval_spline(double xa[], double ya[], double y2a[], int n, double x)
{
    int klo = 0, khi = n - 1;
    while (khi - klo > 1)
    {
        int k = (khi + klo) >> 1;
        if (xa[k] > x)
            khi = k;
        else
            klo = k;
    }
    double h = xa[khi] - xa[klo];
    double a = (xa[khi] - x) / h;
    double b = (x - xa[klo]) / h;
    return (a * ya[klo] + b * ya[khi] +
            ((a * a * a - a) * y2a[klo] +
             (b * b * b - b) * y2a[khi]) * (h * h) / 6.0);
}

void SkinnedEqGraph::draw()
{
    QPixmap pixmap = skin()->getEqPart (Skin::EQ_GRAPH);
    if(pixmap.isNull())
        pixmap = QPixmap(113 * m_ratio, 19 * m_ratio);

    if (m_values.size()!=10)
    {
        setPixmap (pixmap);
        return;
    }

    double x[] = { 0, 11, 23, 35, 47, 59, 71, 83, 97, 109 }, yf[10];
    double *bands = new double[10];

    for (int i = 0; i < 10; ++i)
    {
        bands[i] = m_values.at (i);
    }

    init_spline (x, bands, 10, yf);
    for (int i = 0; i < 113; i++)
    {
        int y = 9 - (int) ((eval_spline (x, bands, yf, 10, i) * 9.0) / 20.0);
        y = qBound(0, y, 18);

        QPainter paint(&pixmap);
        paint.drawPixmap(i * m_ratio, y * m_ratio, skin()->getEqSpline(y));
    }
    setPixmap (pixmap);
    delete [] bands;
}

void SkinnedEqGraph::updateSkin()
{
    m_ratio = skin()->ratio();
    draw();
    setVisible(!skin()->getEqPart(Skin::EQ_GRAPH).isNull());
}
