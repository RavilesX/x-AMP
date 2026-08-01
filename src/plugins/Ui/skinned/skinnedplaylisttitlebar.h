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
#ifndef SKINNEDPLAYLISTTITLEBAR_H
#define SKINNEDPLAYLISTTITLEBAR_H

#include <qmmpui/metadataformatter.h>
#include "skinnedplaylist.h"
#include "pixmapwidget.h"
#include "skinnedmainwindow.h"

class SkinnedMainWindow;
class SkinnedButton;
class PlayListModel;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedPlayListTitleBar : public PixmapWidget
{
Q_OBJECT
public:
    SkinnedPlayListTitleBar(QWidget *parent = nullptr);
    ~SkinnedPlayListTitleBar();
    void setActive(bool);
    void readSettings();

public slots:
    void showCurrent();
    void setModel(PlayListModel *selected, PlayListModel *previous = nullptr);

private slots:
    void updateSkin() override;
    void shade();

private:
    void updatePositions();
    void updatePixmap();
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

    QPoint pos;
    bool m_active = false;
    SkinnedPlayList* m_pl;
    SkinnedMainWindow* m_mw;
    SkinnedButton *m_close;
    SkinnedButton *m_shade = nullptr;
    SkinnedButton *m_shade2 = nullptr;
    bool m_shaded = false;
    bool m_align = false, m_resize = false;
    int m_ratio;
    int m_height;
    PlayListModel* m_model = nullptr;
    QString m_text;
    QString m_truncatedText;
    QFont m_font;
    MetaDataFormatter m_formatter;
};

#endif
