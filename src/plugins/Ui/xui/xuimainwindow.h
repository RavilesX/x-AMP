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
class UpdateChecker;

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
    /*!
     * Asks GitHub whether a newer release exists and reports either way.
     * \b quiet suppresses everything but a positive answer, which is what the
     * check on startup wants: nobody wants a dialog telling them nothing has
     * changed, still less one saying the network was down.
     */
    void checkForUpdates(bool quiet = false);

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
    //built on first use: an interface that never checks never creates it,
    //and creating it is what sets a proxy up
    UpdateChecker *m_updates = nullptr;
    //whether the check in flight was the one on startup, which reports only
    //good news
    bool m_quietCheck = false;
    //kept so closing a companion by its own button unticks it in the menu
    QAction *m_equalizerAction = nullptr;
    QAction *m_playlistAction = nullptr;
    bool m_hideOnClose = false;
};

#endif
