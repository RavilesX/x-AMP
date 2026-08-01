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
#ifndef SKINNEDTITLEBAR_H
#define SKINNEDTITLEBAR_H

#include <QMainWindow>
#include <QPoint>
#include "pixmapwidget.h"
#include "skinnedplaylist.h"

class SkinnedMainWindow;
class QMouseEvent;
class SkinnedButton;
class SymbolDisplay;
class SkinnedTitleBarControl;
class ShadedVisual;
class SkinnedTimeIndicatorModel;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedTitleBar : public PixmapWidget
{
Q_OBJECT
public:
    explicit SkinnedTitleBar(SkinnedTimeIndicatorModel *model, QWidget *parent = nullptr);

    ~SkinnedTitleBar();

    void setActive(bool);

private slots:
    void onModelChanged();
    void updateSkin() override;
    void showMainMenu();
    void shade();

private:
    void updatePositions();
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

    QPoint m_pos;
    SkinnedMainWindow *m_mw;
    SkinnedButton *m_menu;
    SkinnedButton *m_minimize;
    SkinnedButton *m_shade;
    SkinnedButton *m_shade2 = nullptr;
    SkinnedButton *m_close;
    SymbolDisplay *m_currentTime = nullptr;
    QString formatTime (int);
    bool m_shaded = false;
    bool m_align = false;
    SkinnedTitleBarControl *m_control = nullptr;
    ShadedVisual *m_visual = nullptr;
    SkinnedTimeIndicatorModel *m_model;

};



#endif
