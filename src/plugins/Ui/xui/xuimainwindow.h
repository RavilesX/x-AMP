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

#ifndef XUIMAINWINDOW_H
#define XUIMAINWINDOW_H

#include <QWidget>

class QMenu;
class UiHelper;
class SoundCore;
class MediaPlayer;
class PlayListManager;
class XUiPlayerCard;
class XUiEqualizerCard;
class XUiPlaylistCard;

/*!
 * Frameless main window with a title bar of its own.
 *
 * Frameless costs us the window manager's move, resize and snap behaviour,
 * so those are handed back to the compositor through startSystemMove() and
 * startSystemResize() rather than reimplemented by tracking the pointer --
 * that is the only approach that behaves identically on X11 and Wayland.
 */
class XUiMainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit XUiMainWindow(QWidget *parent = nullptr);
    ~XUiMainWindow();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void closeEvent(QCloseEvent *) override;

private slots:
    void showMainMenu();
    void toggleMaximised();
    void updateWindowTitle();
    void showPreferences();

private:
    QWidget *buildTitleBar();
    void createShortcuts();
    void applyCardVisibility();
    /*! Which window edges the pointer is over, for resize cursors. */
    Qt::Edges edgesAt(const QPoint &pos) const;
    void readSettings();
    void writeSettings();

    UiHelper *m_uiHelper;
    SoundCore *m_core;
    MediaPlayer *m_player;
    PlayListManager *m_playListManager;

    QWidget *m_titleBar;
    XUiPlayerCard *m_playerCard;
    XUiEqualizerCard *m_equalizerCard;
    XUiPlaylistCard *m_playlistCard;
    QMenu *m_mainMenu = nullptr;
    bool m_hideOnClose = false;
};

#endif
