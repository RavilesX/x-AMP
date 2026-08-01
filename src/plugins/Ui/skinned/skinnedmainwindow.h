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
#ifndef SKINNEDMAINWINDOW_H
#define SKINNEDMAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <qmmp/output.h>
#include <qmmp/decoder.h>
#include <qmmp/decoderfactory.h>
#include <qmmpui/playlistitem.h>
#include <qmmpui/metadataformatter.h>
#include "skinneddisplay.h"
#include "skinnedtitlebar.h"

class SkinnedPlayList;
class PlayListManager;
class SkinnedEqWidget;
class SkinnedVisualization;
class Skin;
class SoundCore;
class VisualMenu;
class UiHelper;
class MediaPlayer;
class QMenu;
class QKeyEvent;
class QmmpUiSettings;

/**
   @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    SkinnedMainWindow(QWidget *parent = nullptr);

    ~SkinnedMainWindow() = default;

    inline SkinnedPlayList *playlist() const { return m_playlist; }
    inline QMenu *menu() const { return m_mainMenu; }
    inline SkinnedDisplay *mainDisplay() const {  return m_display; }

public slots:
    void playPause();
    void jumpToTrack();
    void toggleVisibility();
    void showAndRaise();

    void addDir();
    void addFile();
    void playFiles();
    void addUrl();

    void loadPlaylist();
    void savePlaylist();
    void about();
    void updateSettings();

private slots:
    void showState(Qmmp::State state);
    void showMetaData();
    void showSettings();
    void restoreWindowTitle();

private:
    //events
    void closeEvent(QCloseEvent *) override;
    void hideEvent(QHideEvent *) override;
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent* ) override;


    void readSettings();
    void writeSettings();
    void createActions();
    SoundCore *m_core;
    QMenu *m_mainMenu;
    SkinnedDisplay *m_display;
    SkinnedPlayList *m_playlist;
    PlayListManager *m_pl_manager;
    SkinnedEqWidget *m_equalizer;
    SkinnedVisualization *m_vis = nullptr;
    bool m_update = false;
    Skin *m_skin;
    bool m_hideOnClose, m_startHidden;
    VisualMenu *m_visMenu;
    UiHelper *m_uiHelper;
    QmmpUiSettings *m_ui_settings;
    MediaPlayer *m_player;
    MetaDataFormatter m_titleFormatter;
};

#endif
