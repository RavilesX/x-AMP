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
#include "xuiwindow.h"

class QAction;
class QLabel;
class QMenu;
class UiHelper;
class SoundCore;
class MediaPlayer;
class PlayListManager;
class XUiPlayerCard;
class XUiEqualizerCard;
class XUiPlaylistCard;

/*!
 * The player window: title bar, player card, and the menu that drives the
 * whole interface.
 *
 * The equalizer and the playlist are top-level windows of their own, so each
 * can be moved and sized apart from the others; XUiDock snaps them back
 * together and carries the docked ones along when this window moves.
 */
class XUiMainWindow : public XUiWindow
{
    Q_OBJECT
public:
    explicit XUiMainWindow(QWidget *parent = nullptr);
    ~XUiMainWindow();

protected:
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void closeEvent(QCloseEvent *) override;

private slots:
    void showMainMenu();
    void toggleMaximised();
    void updateWindowTitle();
    void showPreferences();
    /*! The preferences, opened straight at the scheduler. */
    void showSchedulerSettings();

private:
    QWidget *buildTitleBar();
    /*! Runs the preferences dialog, optionally landing on the scheduler. */
    void openPreferences(bool scheduler);
    void createShortcuts();
    /*! Shows or hides the companion windows to match the settings. */
    void applyCardVisibility();
    /*! Puts a companion away and records that the user asked for it. */
    void hideCompanion(const QString &key, QAction *action);
    /*!
     * Restores the companions' geometry, stacking any that has never been
     * placed under this window so the three read as one column at first run.
     */
    void stackCompanions();
    /*! Re-reads the accent and repaints everything that uses it. */
    void applyAccent();
    void updateWordmark();
    void readSettings();
    void writeSettings();

    UiHelper *m_uiHelper;
    SoundCore *m_core;
    MediaPlayer *m_player;
    PlayListManager *m_playListManager;

    QWidget *m_titleBar;
    QLabel *m_wordmark;
    XUiPlayerCard *m_playerCard;
    XUiEqualizerCard *m_equalizerCard;
    XUiPlaylistCard *m_playlistCard;
    //each companion card lives in its own top-level window, so it can be
    //moved and resized on its own and snapped back against this one
    XUiWindow *m_equalizerWindow = nullptr;
    XUiWindow *m_playlistWindow = nullptr;
    QMenu *m_mainMenu = nullptr;
    //kept so closing a companion by its own button unticks it in the menu
    QAction *m_equalizerAction = nullptr;
    QAction *m_playlistAction = nullptr;
    bool m_hideOnClose = false;
};

#endif
