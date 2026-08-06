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

#ifndef XUITHEME_H
#define XUITHEME_H

#include <QColor>
#include <QPalette>
#include <QString>

/*!
 * Colours and metrics for the whole interface.
 *
 * Everything is drawn with QPainter against these values instead of being
 * loaded from bitmaps, so the interface stays sharp at any size and any
 * device pixel ratio. Put new constants here rather than hard-coding colours
 * in the widgets.
 */
namespace XUi
{
    //surfaces, darkest first
    inline const QColor Background = QColor(0x08, 0x0b, 0x10);
    inline const QColor Card       = QColor(0x0d, 0x11, 0x18);
    inline const QColor CardTop    = QColor(0x11, 0x16, 0x1f); //card gradient start
    inline const QColor Elevated   = QColor(0x16, 0x1c, 0x27); //chips, inset panels
    inline const QColor Border     = QColor(0x1c, 0x23, 0x30);
    inline const QColor Hover      = QColor(0x1f, 0x27, 0x35);

    //Accent. Not const: the user picks this one, and everything else in the
    //interface is a fixed neutral. The bright and deep variants are derived
    //rather than picked, so any colour keeps the same internal relationships
    //the original blue had -- see setAccent().
    inline QColor Accent       = QColor(0x2f, 0x86, 0xf6);
    inline QColor AccentBright = QColor(0x6f, 0xb4, 0xff);
    inline QColor AccentDeep   = QColor(0x0d, 0x47, 0xc4);
    //resting colour of the glyph buttons: the accent, held back enough that a
    //toggled-on button still reads as brighter than a toggled-off one
    inline QColor AccentMuted  = QColor(0x2a, 0x66, 0xaf);

    /*! The blue the interface ships with, for the reset button. */
    inline QColor defaultAccent() { return QColor(0x2f, 0x86, 0xf6); }

    inline const QString AccentKey = QStringLiteral("XUi/accent");

    /*!
     * Sets the accent and derives its two variants.
     *
     * Measured off the original blue: the bright one sits about 14% higher in
     * lightness and the deep one about 17% lower, at the same hue and
     * saturation. Deriving them keeps a chosen colour looking like one family
     * instead of three unrelated picks.
     */
    inline void setAccent(const QColor &colour)
    {
        if(!colour.isValid())
            return;
        Accent = colour;
        const float h = colour.hslHueF() < 0 ? 0.0f : colour.hslHueF(); //greys report -1
        const float s = colour.hslSaturationF();
        const float l = colour.lightnessF();
        AccentBright = QColor::fromHslF(h, s, qMin(1.0f, l + 0.145f));
        AccentDeep = QColor::fromHslF(h, s, qMax(0.0f, l - 0.165f));
        AccentMuted = QColor::fromHslF(h, s * 0.72f, qMax(0.0f, l - 0.13f));
    }

    //text
    inline const QColor Text      = QColor(0xe9, 0xee, 0xf6);
    inline const QColor TextDim   = QColor(0x8b, 0x96, 0xa8);
    inline const QColor TextFaint = QColor(0x4d, 0x57, 0x67);

    //Metrics. Kept deliberately tight: three stacked cards add up fast, and
    //the window's minimum size is derived from them, so every pixel here is
    //a pixel the user cannot shrink the window below.
    inline constexpr int CardRadius       = 12;
    inline constexpr int CardGap          = 10;
    inline constexpr int CardPadding      = 14;
    inline constexpr int WindowRadius     = 14;
    inline constexpr int TitleBarHeight   = 38;
    inline constexpr int CardHeaderHeight = 46;

    /*!
     * Palette matching the cards, for the Qt widgets this interface does not
     * paint itself: menus, tooltips, dialogs, the search field.
     *
     * Without it those follow whatever Qt theme the user runs, so a light
     * desktop theme would put white menus over the dark cards. Applied to the
     * main window, which every popup parented to it inherits.
     */
    inline QPalette palette()
    {
        QPalette p;
        p.setColor(QPalette::Window, Card);
        p.setColor(QPalette::WindowText, Text);
        p.setColor(QPalette::Base, Elevated);
        p.setColor(QPalette::AlternateBase, Card);
        p.setColor(QPalette::Text, Text);
        p.setColor(QPalette::Button, Elevated);
        p.setColor(QPalette::ButtonText, Text);
        p.setColor(QPalette::Highlight, Accent);
        p.setColor(QPalette::HighlightedText, Text);
        p.setColor(QPalette::ToolTipBase, Elevated);
        p.setColor(QPalette::ToolTipText, Text);
        p.setColor(QPalette::PlaceholderText, TextFaint);
        p.setColor(QPalette::Light, Hover);
        p.setColor(QPalette::Mid, Border);
        p.setColor(QPalette::Dark, Background);
        p.setColor(QPalette::Disabled, QPalette::Text, TextFaint);
        p.setColor(QPalette::Disabled, QPalette::ButtonText, TextFaint);
        p.setColor(QPalette::Disabled, QPalette::WindowText, TextFaint);
        return p;
    }

    /*!
     * Style sheet for the Qt widgets this interface does not paint itself.
     *
     * The palette alone is not enough: QStyle draws menus, tooltips and
     * scrollbars with its own frames and gradients, so on a system style they
     * came out grey and boxy next to the cards. Set on the main window, which
     * child widgets and popups parented to it inherit.
     */
    inline QString styleSheet()
    {
        return QStringLiteral(
            "QMenu {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 10px;"
            "  padding: 6px;"
            "}"
            "QMenu::item {"
            "  padding: 7px 30px 7px 14px;"
            "  margin: 1px 2px;"
            "  border-radius: 6px;"
            "  color: %3;"
            "}"
            //neutral highlight, not the accent: white text on a pale accent
            //was unreadable, and a menu row has no room for a marker
            "QMenu::item:selected { background-color: %8; color: %3; }"
            "QMenu::item:disabled { color: %5; }"
            "QMenu::separator { height: 1px; background: %2; margin: 5px 10px; }"
            //checkable entries: a small accent square instead of the style's box
            "QMenu::indicator {"
            "  width: 13px; height: 13px;"
            "  margin-left: 6px;"
            "  border: 1px solid %2;"
            "  border-radius: 3px;"
            "}"
            "QMenu::indicator:checked { background-color: %4; border-color: %4; }"
            "QToolTip {"
            "  background-color: %1;"
            "  color: %3;"
            "  border: 1px solid %2;"
            "  border-radius: 6px;"
            "  padding: 4px 7px;"
            "}"
            "QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }"
            "QScrollBar::handle:vertical {"
            "  background: %2; border-radius: 4px; min-height: 30px;"
            "}"
            "QScrollBar::handle:vertical:hover { background: %4; }"
            "QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }"
            "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }"
            "QLineEdit {"
            "  background: %6; border: 1px solid %2; border-radius: 8px;"
            "  padding: 0 10px; color: %3; selection-background-color: %4;"
            "}"
            "QLineEdit:focus { border-color: %4; }"

            //shared dialogs: track details, preferences. Their layouts stay as
            //libqmmpui builds them; only the surfaces are brought into line.
            //The accent border sets them apart from the window behind, which
            //is the same near-black. No radius: these keep the system title
            //bar, whose corners are square, so a rounded edge would not meet
            //it cleanly.
            "QDialog { background-color: %7; color: %3; border: 2px solid %4; }"
            "QDialog QWidget { background-color: %7; color: %3; }"
            "QTabWidget::pane {"
            "  border: 1px solid %2; border-radius: 8px; top: -1px;"
            "}"
            "QTabBar::tab {"
            "  background: transparent; color: %5;"
            "  padding: 7px 16px; margin-right: 2px;"
            "  border: 1px solid transparent;"
            "  border-top-left-radius: 7px; border-top-right-radius: 7px;"
            "}"
            "QTabBar::tab:hover { color: %3; }"
            "QTabBar::tab:selected {"
            "  color: %3; background: %1; border-color: %2; border-bottom-color: %1;"
            "}"
            "QTextEdit, QPlainTextEdit, QAbstractItemView {"
            "  background: %6; border: 1px solid %2; border-radius: 8px;"
            "  color: %3; selection-background-color: %4;"
            "}"
            //without this the selected row keeps the style's own text colour,
            //which came out dark against the accent
            "QAbstractItemView::item:selected { background: %4; color: #ffffff; }"
            "QAbstractItemView::item:hover { background: %8; }"
            //the settings dialog's page list paints itself (see xuidialogs.cpp):
            //no accent fill, and a gutter on the left for the accent chevron
            "QListWidget#contentsWidget { border: none; background: transparent; }"
            "QListWidget#contentsWidget::item {"
            "  padding: 4px 6px 4px 26px; border-radius: 8px;"
            "}"
            "QListWidget#contentsWidget::item:selected,"
            "QListWidget#contentsWidget::item:hover {"
            "  background: transparent; color: %3;"
            "}"
            "QPushButton, QToolButton {"
            "  background: %6; border: 1px solid %2; border-radius: 7px;"
            "  padding: 6px 14px; color: %3;"
            "}"
            "QPushButton:hover, QToolButton:hover { background: %8; }"
            "QPushButton:pressed, QToolButton:pressed { background: %2; }"
            "QPushButton:disabled, QToolButton:disabled { color: %5; }"
            "QPushButton:default { border-color: %4; }"
            "QCheckBox, QRadioButton, QGroupBox { color: %3; }"
            "QGroupBox {"
            "  border: 1px solid %2; border-radius: 8px;"
            "  margin-top: 10px; padding-top: 8px;"
            "}"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
            "QCheckBox::indicator, QRadioButton::indicator {"
            "  width: 15px; height: 15px;"
            "  border: 1px solid %2; border-radius: 4px; background: %6;"
            "}"
            "QRadioButton::indicator { border-radius: 8px; }"
            "QCheckBox::indicator:checked, QRadioButton::indicator:checked {"
            "  background: %4; border-color: %4;"
            "}"
            "QComboBox, QSpinBox, QDoubleSpinBox {"
            "  background: %6; border: 1px solid %2; border-radius: 7px;"
            "  padding: 4px 8px; color: %3;"
            "}"
            "QComboBox:focus, QSpinBox:focus { border-color: %4; }"
            "QHeaderView::section {"
            "  background: %1; color: %5; border: 0; border-bottom: 1px solid %2;"
            "  padding: 5px 8px;"
            "}")
            .arg(CardTop.name(), Border.name(), Text.name(),
                 Accent.name(), TextFaint.name(), Elevated.name(),
                 Card.name(), Hover.name());
    }
}

#endif
