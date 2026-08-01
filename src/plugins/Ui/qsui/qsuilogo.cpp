/***************************************************************************
 *   Copyright (C) 2011-2026 by Ilya Kotov                                 *
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
#include <QFile>
#include <QTimer>
#include <cmath>
#include <qmmp/qmmp.h>
#include "qsuilogo.h"

QSUiLogo::QSUiLogo(QWidget *parent) : Visual(parent)
{
    QPixmap pixmap(u":/qsui/terminus.png"_s);
    m_letters = {
        { QLatin1Char('0'), pixmap.copy(0, 0, 8, 14) },
        { QLatin1Char('1'), pixmap.copy(8, 0, 8, 14) },
        { QLatin1Char('2'), pixmap.copy(16, 0, 8, 14) },
        { QLatin1Char('3'), pixmap.copy(24, 0, 8, 14) },
        { QLatin1Char('4'), pixmap.copy(32, 0, 8, 14) },
        { QLatin1Char('5'), pixmap.copy(40, 0, 8, 14) },
        { QLatin1Char('6'), pixmap.copy(48, 0, 8, 14) },
        { QLatin1Char('7'), pixmap.copy(56, 0, 8, 14) },
        { QLatin1Char('8'), pixmap.copy(64, 0, 8, 14) },
        { QLatin1Char('9'), pixmap.copy(72, 0, 8, 14) },
        { QLatin1Char('A'), pixmap.copy(80, 0, 8, 14) },
        { QLatin1Char('B'), pixmap.copy(88, 0, 8, 14) },
        { QLatin1Char('C'), pixmap.copy(96, 0, 8, 14) },
        { QLatin1Char('D'), pixmap.copy(104, 0, 8, 14) },
        { QLatin1Char('E'), pixmap.copy(112, 0, 8, 14) },
        { QLatin1Char('F'), pixmap.copy(120, 0, 8, 14) },
        { QLatin1Char('/'), pixmap.copy(128, 0, 8, 14) },
        { QLatin1Char('|'), pixmap.copy(136, 0, 8, 14) },
        { QLatin1Char('\\'), pixmap.copy(144, 0, 8, 14) },
        { QLatin1Char('_'), pixmap.copy(152, 0, 8, 14) },
        { QLatin1Char('-'), pixmap.copy(160, 0, 8, 14) },
        { QLatin1Char('X'), pixmap.copy(168, 0, 8, 14) },
        { QLatin1Char('.'), pixmap.copy(176, 0, 8, 14) },
        { QLatin1Char(' '), pixmap.copy(184, 0, 8, 14) }
    };

    QFile file(u":/ascii_logo.txt"_s);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while(!file.atEnd())
    {
        QString line = QString::fromLatin1(file.readLine());
        m_source_lines.append(line);
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &QSUiLogo::updateLetters);
    m_timer->setInterval(50);

    m_value = 0;
    m_elapsed = 0;
    updateLetters();
    Visual::add(this);
    m_timer->start();
}

QSUiLogo::~QSUiLogo()
{
    Visual::remove(this);
}

void QSUiLogo::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    for(int row = 0; row < m_lines.count(); ++row)
    {
        QString text = m_lines.at(row);
        for(int i = 0; i < text.size(); ++i)
        {
            painter.drawPixmap(width() / 2 - 155 + i * 8, row * 14, m_letters.value(text[i]));
        }
    }
}

void QSUiLogo::mousePressEvent(QMouseEvent *)
{
    m_elapsed = 2000;
    m_value = 0;
}

void QSUiLogo::updateLetters()
{
    if(m_elapsed < 2000)
    {
        m_value = m_elapsed / 100;
        processPreset1();
    }
    else if (m_elapsed < 6000)
    {
        m_value++;
        processPreset2();
    }
    else if(m_elapsed < 9000)
    {
        m_value++;
        processPreset3();
    }
    else if(m_elapsed < 12000)
    {
        processPreset4();
    }
    else
    {
       m_value = 0;
       m_elapsed = 0;
    }
    m_elapsed += 50;
}

void QSUiLogo::processPreset1()
{
    m_lines.clear();
    for(int i = 0; i < m_source_lines.count(); ++i)
    {
        QString line = m_source_lines[i];
        line.replace(QLatin1Char('X'), QLatin1Char('.'));
        if(m_value == i)
        {
            line.remove(0, 2);
            line.append(QChar::Space);
        }
        else if(m_value == i - 1 || m_value == i + 1)
        {
            line.remove(0, 1);
            line.append(QChar::Space);
        }
        m_lines.append(line);
    }
    update();
}

void QSUiLogo::processPreset2()
{
    m_lines.clear();
    QString str = QStringLiteral("..0000..");//.arg(Qmmp::strVersion().left(5));
    int at = m_value % str.size();

    for(QString line : std::as_const(m_source_lines))
    {
        while(line.contains(QLatin1Char('X')))
        {
            at++;
            line.replace(line.indexOf(QLatin1Char('X')), 1, str.at(at % str.size()).toUpper());
        }

        m_lines.append(line);
    }
    update();
}

void QSUiLogo::processPreset3()
{
    m_lines.clear();
    QString str = QStringLiteral("...%1...").arg(Qmmp::strVersion().left(5));
    int at = m_value % str.size();

    for(QString line : std::as_const(m_source_lines))
    {
        while(line.contains(QLatin1Char('X')))
        {
            at++;
            line.replace(line.indexOf(QLatin1Char('X')), 1, str.at(at % str.size()).toUpper());
        }

        m_lines.append(line);
    }
    update();
}

void QSUiLogo::processPreset4()
{
    m_lines.clear();

    int max = 0;

    if(takeData(m_buffer))
    {
        for(int j = 0; j < QMMP_VISUAL_NODE_SIZE; j += 8)
        {
            max = qMax(max, int(std::abs(m_buffer[j] * 65536.0)));
        }
    }

    m_value -= 512;
    m_value = qMax(m_value, max);

    int at = 0;

    for(QString line : std::as_const(m_source_lines))
    {
        int count = line.count(QLatin1Char('X'));
        int k = 0;

        while(k < m_value * count / 65536 / 2)
        {
            int value = std::abs(m_buffer[qMin(at++, QMMP_VISUAL_NODE_SIZE)] * 16);
            line.replace(line.indexOf(QLatin1Char('X')), 1, QStringLiteral("%1").arg(value, 0, 16).toUpper());
            k++;
        }

        k = 0;

        while(k < m_value * count / 65536 / 2)
        {
            int value = std::abs(m_buffer[qMin(at++, QMMP_VISUAL_NODE_SIZE)] * 16);
            line.replace(line.lastIndexOf(QLatin1Char('X')), 1, QStringLiteral("%1").arg(value, 0, 16).toUpper());
            k++;
        }

        while(line.contains(QLatin1Char('X')))
        {
            line.replace(line.indexOf(QLatin1Char('X')), 1, QLatin1Char('.'));
        }

        m_lines.append(line);
    }
    update();
}
