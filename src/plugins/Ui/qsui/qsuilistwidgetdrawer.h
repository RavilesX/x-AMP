/***************************************************************************
 *   Copyright (C) 2015-2026 by Ilya Kotov                                 *
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

#ifndef QSUILISTWIDGETDRAWER_H
#define QSUILISTWIDGETDRAWER_H

#include <QString>
#include <QStringList>
#include <QColor>
#include <QRect>
#include <QFontMetrics>
#include <QImage>

class QPainter;
class PlayListHeaderModel;
class QmmpUiSettings;

struct QSUiListWidgetRow
{
    QStringList titles;
    QList<int> sizes;
    QList<int> alignment;
    QImage cover;
    QString length;
    QString extraString;
    int number = 0;
    int numberColumnWidth = 0;
    int lengthColumnWidth = 0;
    int trackStateColumn = 0;
    int line = 0;
    int subIndex = 0;
    bool alternateColor = false;
    enum
    {
        NO_FLAGS = 0x00,
        GROUP = 0x01,
        SELECTED = 0x02,
        CURRENT = 0x04,
        ANCHOR = 0x08
    };

    enum
    {
        ALIGN_LEFT = 0,
        ALIGN_CENTER,
        ALIGN_RIGHT,
    };

    int flags = NO_FLAGS;
    QRect rect; //geometry
    bool autoResize = false;
};

/**
   @author Ilya Kotov <forkotov02@ya.ru>
*/
class QSUiListWidgetDrawer
{
public:
    QSUiListWidgetDrawer();
    ~QSUiListWidgetDrawer();

    void readSettings();
    void loadSystemColors();
    int rowHeight() const;
    int numberWidth() const;
    void calculateNumberWidth(int count);
    void setSingleColumnMode(int enabled);
    void prepareRow(QSUiListWidgetRow *row);
    void fillBackground(QPainter *painter, int width, int height);
    void drawBackground(QPainter *painter, QSUiListWidgetRow *row);
    void drawSeparator(QPainter *painter, QSUiListWidgetRow *row, bool rtl);
    void drawMultiLineSeparator(QPainter *painter, QSUiListWidgetRow *row, bool rtl);
    void drawTrack(QPainter *painter, QSUiListWidgetRow *row, bool rtl);
    void drawDropLine(QPainter *painter, int y, int width);

private:

    enum Font
    {
        MAIN_FONT_NORMAL = 0,
        MAIN_FONT_BOLD,
        MAIN_FONT_EXTRA,
        PL_GROUP_FONT,
        PL_GROUP_FONT_EXTRA
    };
    QFont m_fonts[PL_GROUP_FONT_EXTRA + 1];
    QFontMetrics *m_metrics[PL_GROUP_FONT_EXTRA + 1] = { nullptr };
    QColor m_normal, m_current, m_normal_bg, m_selected_bg, m_alternate_bg, m_highlighted, m_splitter;
    QColor m_group_bg, m_group_alt_bg, m_group_text, m_current_bg, m_current_alt_bg;
    PlayListHeaderModel *m_header_model;
    QmmpUiSettings *m_ui_settings;
    QImage m_emptyCover;
    bool m_show_numbers = false;
    bool m_show_anchor = false;
    bool m_align_numbers = false;
    bool m_show_lengths = false;
    bool m_single_column = true;
    bool m_show_splitters = true;
    int m_padding = 0;
    int m_number_width = 0;
    int m_row_height = 0;
};

#endif // QSUILISTWIDGETDRAWER_H
