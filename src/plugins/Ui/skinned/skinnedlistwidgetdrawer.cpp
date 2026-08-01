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

#include <QSettings>
#include <QPainter>
#include <QApplication>
#include <qmmp/qmmp.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/qmmpuisettings.h>
#include "skin.h"
#include "skinnedlistwidgetdrawer.h"

// |= number=|=row1=|=row2=|=extra= duration=|

SkinnedListWidgetDrawer::SkinnedListWidgetDrawer() :
    m_emptyCover(u":/skinned/ui_no_cover.png"_s)

{
    m_header_model = PlayListManager::instance()->headerModel();
    m_ui_settings = QmmpUiSettings::instance();
    readSettings();
}

SkinnedListWidgetDrawer::~SkinnedListWidgetDrawer()
{
    for(int i = 0; i <= PL_GROUP_FONT_EXTRA; ++i)
        delete m_metrics[i];
}

void SkinnedListWidgetDrawer::readSettings()
{
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    m_show_anchor = settings.value("pl_show_anchor"_L1, false).toBool();
    m_show_numbers = settings.value ("pl_show_numbers"_L1, true).toBool();
    m_show_splitters = settings.value("pl_show_splitters"_L1, true).toBool();
    m_show_lengths = settings.value ("pl_show_lengths"_L1, true).toBool();
    m_align_numbers = settings.value ("pl_align_numbers"_L1, false).toBool();

    QFont defaultFont = qApp->font();
    m_fonts[MAIN_FONT_NORMAL] = defaultFont;
    m_fonts[PL_GROUP_FONT] = defaultFont;
    m_fonts[PL_GROUP_FONT_EXTRA] = defaultFont;
    m_fonts[PL_GROUP_FONT_EXTRA].setPointSize(defaultFont.pointSize() - 1);
    m_fonts[PL_GROUP_FONT_EXTRA].setStyle(QFont::StyleItalic);

    if(!settings.value(u"use_system_fonts"_s, false).toBool())
    {
        m_fonts[MAIN_FONT_NORMAL].fromString(settings.value(u"pl_font"_s, defaultFont.toString()).toString());
        m_fonts[PL_GROUP_FONT].fromString(settings.value(u"pl_group_font"_s, defaultFont.toString()).toString());
        m_fonts[PL_GROUP_FONT_EXTRA].fromString(settings.value(u"pl_extra_row_font"_s, m_fonts[PL_GROUP_FONT_EXTRA].toString()).toString());
    }

    //m_fonts[MAIN_FONT_BOLD].setBold(true);
    m_fonts[MAIN_FONT_EXTRA] = m_fonts[MAIN_FONT_NORMAL];
    m_fonts[MAIN_FONT_EXTRA].setPointSize(m_fonts[MAIN_FONT_NORMAL].pointSize() - 1);

    {
        Skin *skin = Skin::instance();
        bool alternate_splitter_color = settings.value("pl_alt_splitter_color"_L1, false).toBool();
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        m_normal = QColor::fromString(skin->getPLValue("normal"));
        m_current = QColor::fromString(skin->getPLValue("current"));
        m_highlighted = m_normal;
        m_normal_bg = QColor::fromString(skin->getPLValue("normalbg"));
        m_selected_bg = QColor::fromString(skin->getPLValue("selectedbg"));
#else
        m_normal.setNamedColor(skin->getPLValue("normal"));
        m_current.setNamedColor(skin->getPLValue("current"));
        m_highlighted = m_normal;
        m_normal_bg.setNamedColor(skin->getPLValue("normalbg"));
        m_selected_bg.setNamedColor(skin->getPLValue("selectedbg"));
#endif
        m_alternate_bg = m_normal_bg;
        m_splitter = alternate_splitter_color ? m_current : m_normal;
        m_group_bg = m_normal_bg;
        m_group_alt_bg = m_normal_bg;
        m_group_text = m_normal;
        m_current_bg = m_normal_bg;
        m_current_alt_bg = m_normal_bg;
    }

    if(!settings.value("pl_use_skin_colors"_L1, true).toBool())
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        m_normal_bg = QColor::fromString(settings.value("pl_bg1_color"_L1, m_normal_bg.name()).toString());
        m_alternate_bg = QColor::fromString(settings.value("pl_bg2_color"_L1, m_alternate_bg.name()).toString());
        m_selected_bg = QColor::fromString(settings.value("pl_highlight_color"_L1, m_selected_bg.name()).toString());
        m_normal = QColor::fromString(settings.value("pl_normal_text_color"_L1, m_normal.name()).toString());
        m_current = QColor::fromString(settings.value("pl_current_text_color"_L1, m_current.name()).toString());
        m_highlighted = QColor::fromString(settings.value(u"pl_hl_text_color"_s, m_highlighted.name()).toString());
        m_splitter = QColor::fromString(settings.value("pl_splitter_color"_L1, m_splitter).toString());
        m_group_text = QColor::fromString(settings.value("pl_group_text"_L1, m_group_text.name()).toString());
#else
        m_normal_bg.setNamedColor(settings.value("pl_bg1_color"_L1, m_normal_bg.name()).toString());
        m_alternate_bg.setNamedColor(settings.value("pl_bg2_color"_L1, m_alternate_bg.name()).toString());
        m_selected_bg.setNamedColor(settings.value("pl_highlight_color"_L1, m_selected_bg.name()).toString());
        m_normal.setNamedColor(settings.value("pl_normal_text_color"_L1, m_normal.name()).toString());
        m_current.setNamedColor(settings.value("pl_current_text_color"_L1, m_current.name()).toString());
        m_highlighted.setNamedColor(settings.value(u"pl_hl_text_color"_s, m_highlighted.name()).toString());
        m_splitter.setNamedColor(settings.value("pl_splitter_color"_L1, m_splitter).toString());
        m_group_text.setNamedColor(settings.value("pl_group_text"_L1, m_group_text.name()).toString());
#endif
        m_group_bg = m_normal_bg;
        m_group_alt_bg = m_alternate_bg;
        m_current_bg = m_normal_bg;
        m_current_alt_bg = m_alternate_bg;
    }

    if(settings.value("pl_override_group_bg"_L1, false).toBool())
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        m_group_bg = QColor::fromString(settings.value("pl_group_bg"_L1, m_normal_bg.name()).toString());
#else
        m_group_bg.setNamedColor(settings.value("pl_group_bg"_L1, m_normal_bg.name()).toString());
#endif
        m_group_alt_bg = m_group_bg;
    }

    if(settings.value("pl_override_current_bg"_L1, false).toBool())
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        m_current_bg = QColor::fromString(settings.value("pl_current_bg_color"_L1, m_normal_bg.name()).toString());
#else
        m_current_bg.setNamedColor(settings.value("pl_current_bg_color"_L1, m_normal_bg.name()).toString());
#endif
        m_current_alt_bg = m_current_bg;
    }

    settings.endGroup();

    for(int i = 0; i <= PL_GROUP_FONT_EXTRA; ++i)
    {
        delete m_metrics[i];
        m_metrics[i] = new QFontMetrics(m_fonts[i]);
    }

    m_padding = m_metrics[MAIN_FONT_NORMAL]->horizontalAdvance(u"9"_s) / 2;
    m_row_height = m_metrics[MAIN_FONT_NORMAL]->lineSpacing() + 1;
}

int SkinnedListWidgetDrawer::rowHeight() const
{
    return m_row_height;
}

int SkinnedListWidgetDrawer::numberWidth() const
{
    return m_number_width;
}

void SkinnedListWidgetDrawer::calculateNumberWidth(int count)
{
    //song numbers width
    if(m_show_numbers && m_align_numbers && count)
        m_number_width = m_metrics[MAIN_FONT_NORMAL]->horizontalAdvance(u"9"_s) * QString::number(count).size();
    else
        m_number_width = 0;
}

void SkinnedListWidgetDrawer::setSingleColumnMode(int enabled)
{
    m_single_column = enabled;
}

void SkinnedListWidgetDrawer::prepareRow(SkinnedListWidgetRow *row)
{
    if(m_number_width && m_single_column)
        row->numberColumnWidth = m_number_width + 2 * m_padding;
    else
        row->numberColumnWidth = 0;

    if(row->flags & SkinnedListWidgetRow::GROUP)
    {
        row->titles[0] = m_metrics[MAIN_FONT_NORMAL]->elidedText (row->titles[0], Qt::ElideRight,
                row->rect.width() - m_number_width - 12 - 70);
        return;
    }

    QFontMetrics *metrics = m_metrics[MAIN_FONT_NORMAL];

    if(row->titles.count() == 1)
    {
        if(m_show_numbers && !m_align_numbers)
            row->titles[0].prepend(QStringLiteral("%1. ").arg(row->number));

        if((m_show_lengths && !row->length.isEmpty()) || !row->extraString.isEmpty())
            row->lengthColumnWidth = m_padding;
        else
            row->lengthColumnWidth = 0;

        if(m_show_lengths && !row->length.isEmpty())
            row->lengthColumnWidth += metrics->horizontalAdvance(row->length) + m_padding;

        if(!row->extraString.isEmpty())
            row->lengthColumnWidth += m_metrics[MAIN_FONT_EXTRA]->horizontalAdvance(row->extraString) + m_padding;
    }

    //elide title
    int visible_width = row->rect.width() - row->lengthColumnWidth - row->numberColumnWidth;

    if(row->titles.count() == 1 && !row->lengthColumnWidth)
    {
        row->titles[0] = metrics->elidedText (row->titles[0], Qt::ElideRight, visible_width - 2 * m_padding);
        return;
    }

    if(row->titles.count() == 1)
    {
        row->titles[0] = metrics->elidedText (row->titles[0], Qt::ElideRight, visible_width - m_padding);
        return;
    }

    for(int i = 0; i < row->titles.count(); ++i)
    {
        int size = row->sizes[i];
        if(i == row->trackStateColumn && !row->extraString.isEmpty())
        {
            int text_size = qMax(0, size - 3 * m_padding - m_metrics[MAIN_FONT_EXTRA]->horizontalAdvance(row->extraString));
            row->titles[i] = metrics->elidedText (row->titles[i], Qt::ElideRight, text_size);
            row->extraString = m_metrics[MAIN_FONT_EXTRA]->elidedText(row->extraString, Qt::ElideRight,
                                                                      size - 3 * m_padding - m_metrics[MAIN_FONT_NORMAL]->horizontalAdvance(row->titles[i]));
        }
        else
        {
            row->titles[i] = metrics->elidedText(row->titles[i], Qt::ElideRight, size - 2 * m_padding);
        }
    }
}

void SkinnedListWidgetDrawer::fillBackground(QPainter *painter, int width, int height)
{
    painter->setBrush(m_normal_bg);
    painter->setPen(m_normal_bg);
    painter->drawRect(0, 0, width, height);
}

void SkinnedListWidgetDrawer::drawBackground(QPainter *painter, SkinnedListWidgetRow *row)
{
    if(row->flags & SkinnedListWidgetRow::SELECTED)
    {
        painter->setBrush(m_selected_bg);
    }
    else if(row->flags & SkinnedListWidgetRow::GROUP)
    {
        if(row->alternateColor)
        {
            painter->setBrush(QBrush(m_group_alt_bg));
            painter->setPen(m_group_alt_bg);
        }
        else
        {
            painter->setBrush(QBrush(m_group_bg));
            painter->setPen(m_group_bg);
        }
    }
    else if(row->flags & SkinnedListWidgetRow::CURRENT)
    {
        if(row->alternateColor)
        {
            painter->setBrush(QBrush(m_current_alt_bg));
            painter->setPen(m_current_alt_bg);
        }
        else
        {
            painter->setBrush(QBrush(m_current_bg));
            painter->setPen(m_current_bg);
        }
    }
    else
    {
        if(row->alternateColor)
        {
            painter->setBrush(QBrush(m_alternate_bg));
            painter->setPen(m_alternate_bg);
        }
        else
        {
            painter->setBrush(QBrush(m_normal_bg));
            painter->setPen(m_normal_bg);
        }
    }

    if(m_show_anchor && (row->flags & SkinnedListWidgetRow::ANCHOR))
    {
        painter->setPen(m_normal);
    }
    else if(row->flags & SkinnedListWidgetRow::SELECTED)
    {
        painter->setPen(m_selected_bg);
    }
    painter->drawRect(row->rect);
}

void SkinnedListWidgetDrawer::drawSeparator(QPainter *painter, SkinnedListWidgetRow *row, bool rtl)
{
    int sx = rtl ? (row->rect.right() - 50 - row->numberColumnWidth - m_metrics[MAIN_FONT_NORMAL]->horizontalAdvance(row->titles[0])) :
        (row->rect.x() + row->numberColumnWidth + 50);
    int sy = row->rect.y() + m_metrics[MAIN_FONT_NORMAL]->overlinePos() - 1;
    bool dividingLine = m_ui_settings->groupDividingLineVisible();

    painter->setFont(m_fonts[MAIN_FONT_NORMAL]);
    painter->setPen((row->flags & SkinnedListWidgetRow::SELECTED) ? m_highlighted : m_group_text);
    painter->drawText(sx, sy, row->titles[0]);

    sy -= m_metrics[MAIN_FONT_NORMAL]->lineSpacing() / 2 - 2;

    if(rtl)
    {
        if(dividingLine)
        {
            painter->drawLine(row->rect.x() + 5, sy, sx - 5, sy);
            painter->drawLine(sx + m_metrics[MAIN_FONT_NORMAL]->horizontalAdvance(row->titles[0]) + 5, sy,
                    row->rect.right() - row->numberColumnWidth - m_padding, sy);
        }
        if(m_show_splitters && row->numberColumnWidth)
        {
            painter->setPen(m_splitter);
            painter->drawLine(row->rect.right() - row->numberColumnWidth, row->rect.top(),
                              row->rect.right() - row->numberColumnWidth, row->rect.bottom() + 1);
        }
    }
    else
    {
        if(dividingLine)
        {
            painter->drawLine(sx - 45, sy, sx - 5, sy);
            painter->drawLine(sx + m_metrics[MAIN_FONT_NORMAL]->horizontalAdvance(row->titles[0]) + 5, sy,
                    row->rect.width(), sy);
        }
        if(m_show_splitters && row->numberColumnWidth)
        {
            painter->setPen(m_splitter);
            painter->drawLine(row->rect.left() + row->numberColumnWidth, row->rect.top(),
                              row->rect.left() + row->numberColumnWidth, row->rect.bottom() + 1);
        }
    }
}

void SkinnedListWidgetDrawer::drawMultiLineSeparator(QPainter *painter, SkinnedListWidgetRow *row, bool rtl)
{
    int sx = rtl ? (row->rect.right() - row->numberColumnWidth - m_padding - m_metrics[MAIN_FONT_NORMAL]->horizontalAdvance(row->titles[0])) :
        (row->rect.x() + m_padding + row->numberColumnWidth);

    bool extraRowVisible = m_ui_settings->groupExtraRowVisible();
    bool coverVisible = m_ui_settings->groupCoverVisible();
    bool dividingLine = m_ui_settings->groupDividingLineVisible();
    int cy = row->rect.y() + row->rect.height() / 2; //center

    painter->setFont(m_fonts[PL_GROUP_FONT]);
    painter->setPen((row->flags & SkinnedListWidgetRow::SELECTED) ? m_highlighted : m_group_text);

    if(coverVisible)
    {
        if(rtl && row->numberColumnWidth)
            sx -= m_padding;
        else if(row->numberColumnWidth)
            sx += m_padding;

        QImage img = row->cover.isNull() ? m_emptyCover : row->cover;
        painter->drawImage(QRect(sx, row->rect.y() + m_padding,
                                 row->rect.height() - 2 * m_padding, row->rect.height() - 2 * m_padding), img);
        sx += row->rect.height();
    }

    if(extraRowVisible)
    {
        int offset = dividingLine ? m_padding : (m_padding / 2);
        painter->drawText(sx, cy - m_metrics[PL_GROUP_FONT]->underlinePos() - offset, row->titles[0]);
        painter->setFont(m_fonts[PL_GROUP_FONT_EXTRA]);
        painter->drawText(sx, cy + m_metrics[PL_GROUP_FONT_EXTRA]->overlinePos() + offset, row->titles[1]);

        if(dividingLine)
            painter->drawLine(sx, cy, row->rect.x() + row->rect.width() - 2 * m_padding, cy);
    }
    else
    {
        painter->drawText(sx, cy + m_metrics[PL_GROUP_FONT]->strikeOutPos(), row->titles[0]);

        if(dividingLine)
        {
            sx += m_metrics[PL_GROUP_FONT]->horizontalAdvance(row->titles[0]) + m_padding;
            painter->drawLine(sx, cy, row->rect.x() + row->rect.width() - 10, cy);
        }
    }


    if(rtl)
    {
        if(m_show_splitters && row->numberColumnWidth)
        {
            painter->setPen(m_splitter);
            painter->drawLine(row->rect.right() - row->numberColumnWidth, row->rect.top(),
                              row->rect.right() - row->numberColumnWidth, row->rect.bottom() + 1);
        }
    }
    else
    {
        if(m_show_splitters && row->numberColumnWidth)
        {
            painter->setPen(m_splitter);
            painter->drawLine(row->rect.left() + row->numberColumnWidth, row->rect.top(),
                              row->rect.left() + row->numberColumnWidth, row->rect.bottom() + 1);
        }
    }
}

void SkinnedListWidgetDrawer::drawTrack(QPainter *painter, SkinnedListWidgetRow *row, bool rtl)
{
    int sy = row->rect.y() + m_metrics[MAIN_FONT_NORMAL]->overlinePos() - 1;
    int sx = rtl ? row->rect.right() : row->rect.x();
    int title_x = 0, extra_x = 0;
    bool draw_extra = false;

    QFontMetrics *metrics = nullptr;
    QColor textColor;

    painter->setFont(m_fonts[MAIN_FONT_NORMAL]);
    metrics = m_metrics[MAIN_FONT_NORMAL];

    if(row->flags & SkinnedListWidgetRow::CURRENT)
    {
        textColor = m_current;
    }
    else
    {
        textColor = (row->flags & SkinnedListWidgetRow::SELECTED) ? m_highlighted : m_normal;
    }

    painter->setPen(textColor);

    if(rtl)
    {
        //|=duration=extra=|=  col1=|=number =|
        if(row->titles.count() == 1)
        {
            if(row->numberColumnWidth)
            {
                sx -= row->numberColumnWidth;
                QString number = QString::number(row->number);
                painter->drawText(sx + m_padding, sy, number);
                if(m_show_splitters)
                {
                    painter->setPen(m_splitter);
                    painter->drawLine(sx, row->rect.top(), sx, row->rect.bottom() + 1);
                    painter->setPen(textColor);
                }
            }

            sx -= metrics->horizontalAdvance(row->titles[0]);
            painter->drawText(sx - m_padding, sy, row->titles[0]);
            sx = row->rect.x() + m_padding;

            if(m_show_lengths && !row->length.isEmpty())
            {
                painter->drawText(sx, sy, row->length);
                sx += metrics->horizontalAdvance(row->length);
                sx += m_padding;
            }

            if(!row->extraString.isEmpty())
            {
                painter->setFont(m_fonts[MAIN_FONT_EXTRA]);
                painter->drawText(sx, sy, row->extraString);
            }
        }
        else //|=extra col1=|=  col2=|
        {
            for(int i = 0; i < row->sizes.count(); i++)
            {
                painter->setPen(textColor);
                draw_extra = (i == row->trackStateColumn && !row->extraString.isEmpty());

                if(row->alignment[i] == SkinnedListWidgetRow::ALIGN_LEFT)
                {
                    title_x = sx - row->sizes[i] + m_padding;
                    extra_x = draw_extra ? sx - m_padding - m_metrics[MAIN_FONT_EXTRA]->horizontalAdvance(row->extraString) : 0;
                }
                else if(row->alignment[i] == SkinnedListWidgetRow::ALIGN_RIGHT)
                {
                    title_x = sx - m_padding - metrics->horizontalAdvance(row->titles[i]);
                    extra_x = draw_extra ? sx - row->sizes[i] + m_padding : 0;
                }
                else
                {
                    title_x = sx - row->sizes[i] / 2 - metrics->horizontalAdvance(row->titles[i]) / 2 +
                            (draw_extra ? (m_metrics[MAIN_FONT_EXTRA]->horizontalAdvance(row->extraString) + m_padding) / 2 : 0);
                    extra_x = draw_extra ? title_x - metrics->horizontalAdvance(row->extraString) - m_padding : 0;
                }

                painter->drawText(title_x, sy, row->titles[i]);

                if(draw_extra)
                {
                    QFont prev_font = painter->font();
                    painter->setFont(m_fonts[MAIN_FONT_EXTRA]);
                    painter->drawText(extra_x, sy, row->extraString);
                    painter->setFont(prev_font);
                }

                sx -= row->sizes[i];

                if(m_show_splitters && (!row->autoResize || i < row->sizes.count() - 1)) //do not draw last vertical line
                {
                    painter->setPen(m_splitter);
                    painter->drawLine(sx - 1, row->rect.top(), sx - 1, row->rect.bottom() + 1);
                }
            }
        }
    }
    else
    {
        //|= number=|=col  =|=extra=duration=|
        if(row->titles.count() == 1)
        {
            if(row->numberColumnWidth)
            {
                sx += row->numberColumnWidth;
                QString number = QString::number(row->number);
                painter->drawText(sx - m_padding - metrics->horizontalAdvance(number), sy, number);
                if(m_show_splitters)
                {
                    painter->setPen(m_splitter);
                    painter->drawLine(sx, row->rect.top(), sx, row->rect.bottom() + 1);
                    painter->setPen(textColor);
                }
            }

            painter->drawText(sx + m_padding, sy, row->titles[0]);
            sx = row->rect.right() - m_padding;

            if(m_show_lengths && !row->length.isEmpty())
            {
                sx -= metrics->horizontalAdvance(row->length);
                painter->drawText(sx, sy, row->length);
                sx -= m_padding;
            }

            if(!row->extraString.isEmpty())
            {
                sx -= m_metrics[MAIN_FONT_EXTRA]->horizontalAdvance(row->extraString);
                painter->setFont(m_fonts[MAIN_FONT_EXTRA]);
                painter->drawText(sx, sy, row->extraString);
            }
        }
        else //|=col1  extra=|=col2  =|
        {
            for(int i = 0; i < row->sizes.count(); i++)
            {
                painter->setPen(textColor);
                draw_extra = (i == row->trackStateColumn && !row->extraString.isEmpty());

                if(row->alignment[i] == SkinnedListWidgetRow::ALIGN_LEFT)
                {
                    title_x = sx + m_padding;
                    extra_x = draw_extra ? sx + row->sizes[i] - m_padding - m_metrics[MAIN_FONT_EXTRA]->horizontalAdvance(row->extraString) : 0;
                }
                else if(row->alignment[i] == SkinnedListWidgetRow::ALIGN_RIGHT)
                {
                    title_x = sx + row->sizes[i] - m_padding - metrics->horizontalAdvance(row->titles[i]);
                    extra_x = draw_extra ? sx + m_padding : 0;
                }
                else
                {
                    title_x = sx + row->sizes[i] / 2 - metrics->horizontalAdvance(row->titles[i]) / 2 -
                            (draw_extra ? (m_metrics[MAIN_FONT_EXTRA]->horizontalAdvance(row->extraString) + m_padding) / 2 : 0);
                    extra_x = draw_extra ? title_x + metrics->horizontalAdvance(row->titles[i]) + m_padding : 0;
                }

                painter->drawText(title_x, sy, row->titles[i]);

                if(draw_extra)
                {
                    QFont prev_font = painter->font();
                    painter->setFont(m_fonts[MAIN_FONT_EXTRA]);
                    painter->drawText(extra_x, sy, row->extraString);
                    painter->setFont(prev_font);
                }

                sx += row->sizes[i];

                if(m_show_splitters && (!row->autoResize || i < row->sizes.count() - 1)) //do not draw last vertical line
                {
                    painter->setPen(m_splitter);
                    painter->drawLine(sx - 1, row->rect.top(), sx - 1, row->rect.bottom() + 1);
                }
            }
        }
    }
}

void SkinnedListWidgetDrawer::drawDropLine(QPainter *painter, int y, int width)
{
    painter->setPen(m_current);
    painter->drawLine(5, y, width - 5 , y);
}
