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

#include <QFileDialog>
#include <QDir>
#include <QAction>
#include <QMenu>
#include <QScreen>
#include <algorithm>
#include <math.h>
#include <qmmp/soundcore.h>
#include <qmmp/visual.h>
#include <qmmp/metadatamanager.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/general.h>
#include <qmmpui/playlistparser.h>
#include <qmmpui/playlistformat.h>
#include <qmmpui/commandlinemanager.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/configdialog.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/visualmenu.h>
#include "skinnedhotkeyeditor.h"
#include "skinnedsettings.h"
#include "skinnedmainwindow.h"
#include "skin.h"
#include "skinnedplaylist.h"
#include "dock.h"
#include "skinnedeqwidget.h"
#include "skinnedvisualization.h"
#include "skinnedlistwidget.h"
#include "windowsystem.h"
#include "skinnedactionmanager.h"

SkinnedMainWindow::SkinnedMainWindow(QWidget *parent) : QMainWindow(parent)
{
#ifdef QMMP_WS_X11
    qCDebug(plugin, "detected wm: %s", qPrintable(WindowSystem::netWindowManagerName()));
    QString wm_name = WindowSystem::netWindowManagerName();
    if(wm_name.contains(u"Marco"_s, Qt::CaseInsensitive) ||
            wm_name.contains(u"Metacity"_s, Qt::CaseInsensitive) ||
            wm_name.contains(u"Mutter"_s, Qt::CaseInsensitive) ||
            wm_name.contains(u"GNOME"_s, Qt::CaseInsensitive))
    {
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                       Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint);
    }
    else
#endif
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                       Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint | Qt::WindowSystemMenuHint);

    restoreWindowTitle();

    m_titleFormatter.setPattern(u"%if(%p,%p - %t,%t)"_s);

    new SkinnedActionManager(this);

    m_player = MediaPlayer::instance();
    m_core = SoundCore::instance();
    m_pl_manager = PlayListManager::instance();
    m_uiHelper = UiHelper::instance();
    m_ui_settings = QmmpUiSettings::instance();

    //user interface
    m_skin = new Skin(this);
    setFixedSize(m_skin->scaled(275), m_skin->scaled(116));

    Dock *dock = new Dock(this);
    dock->setMainWidget(this);
    m_display = new SkinnedDisplay(this);
    setCentralWidget(m_display);
    m_display->setFocus ();

    m_playlist = new SkinnedPlayList(m_pl_manager, this);
    dock->addWidget(m_playlist);

    m_equalizer = new SkinnedEqWidget(this);
    dock->addWidget(m_equalizer);

    createActions();
    //prepare visualization
    Visual::initialize(this, m_visMenu, &VisualMenu::updateActions);
    m_vis = SkinnedVisualization::instance();
    Visual::add(m_vis);
    //connections
    connect(m_player, &MediaPlayer::playbackFinished, this, &SkinnedMainWindow::restoreWindowTitle);
    connect(m_playlist, &SkinnedPlayList::next, m_player, &MediaPlayer::next);
    connect(m_playlist, &SkinnedPlayList::prev, m_player, &MediaPlayer::previous);
    connect(m_playlist, &SkinnedPlayList::play, m_player, &MediaPlayer::play);
    connect(m_playlist, &SkinnedPlayList::pause, m_player, &MediaPlayer::pause);
    connect(m_playlist, &SkinnedPlayList::stop, m_player, &MediaPlayer::stop);
    connect(m_playlist, &SkinnedPlayList::eject, this, &SkinnedMainWindow::playFiles);
    connect(m_playlist, &SkinnedPlayList::loadPlaylist, this, &SkinnedMainWindow::loadPlaylist);
    connect(m_playlist, &SkinnedPlayList::savePlaylist, this, &SkinnedMainWindow::savePlaylist);

    connect(m_display, &SkinnedDisplay::shuffleToggled, m_ui_settings, &QmmpUiSettings::setShuffle);
    connect(m_display, &SkinnedDisplay::repeatableToggled, m_ui_settings, &QmmpUiSettings::setRepeatableList);

    connect(m_core, &SoundCore::stateChanged, this, &SkinnedMainWindow::showState);
    connect(m_core, &SoundCore::elapsedChanged, m_playlist, &SkinnedPlayList::setTime);
    connect(m_core, &SoundCore::trackInfoChanged, this, &SkinnedMainWindow::showMetaData);
    connect(m_uiHelper, &UiHelper::toggleVisibilityCalled, this, &SkinnedMainWindow::toggleVisibility);
    connect(m_uiHelper, &UiHelper::showMainWindowCalled, this, &SkinnedMainWindow::showAndRaise);

    readSettings();
    m_display->setEQ(m_equalizer);
    m_display->setPL(m_playlist);
    dock->updateDock();
    m_pl_manager->currentPlayList()->doCurrentVisibleRequest();
    if(m_startHidden && m_uiHelper->visibilityControl())
        toggleVisibility();
}

void SkinnedMainWindow::showState(Qmmp::State state)
{
    switch(state)
    {
    case Qmmp::Playing:
        if(m_pl_manager->currentPlayList()->currentTrack())
            m_equalizer->loadPreset(m_pl_manager->currentPlayList()->currentTrack()->path().section(QLatin1Char('/'), -1));
        break;
    case Qmmp::Paused:
        break;
    case Qmmp::Stopped:
        m_playlist->setTime(-1);
        break;
    default:
        ;
    }
}

void SkinnedMainWindow::showMetaData()
{
    PlayListTrack *track = m_pl_manager->currentPlayList()->currentTrack();
    if(track && track->path() == m_core->trackInfo().path())
    {
        setWindowTitle(m_titleFormatter.format(track));
    }
}

void SkinnedMainWindow::closeEvent(QCloseEvent *)
{
    if(!m_hideOnClose || !m_uiHelper->visibilityControl())
        m_uiHelper->exit();
    else
    {
        m_playlist->close();
        m_equalizer->close();
    }
}

void SkinnedMainWindow::hideEvent(QHideEvent *)
{
    writeSettings();
    m_playlist->writeSettings();
    m_equalizer->writeSettings();
}

void SkinnedMainWindow::addDir()
{
    m_uiHelper->addDirectory(this);
}

void SkinnedMainWindow::addFile()
{
    m_uiHelper->addFiles(this);
}

void SkinnedMainWindow::playFiles()
{
    m_uiHelper->playFiles(this);
}

void SkinnedMainWindow::changeEvent (QEvent * event)
{
    if(event->type() == QEvent::ActivationChange)
    {
        m_display->setActive(isActiveWindow());
    }
}

void SkinnedMainWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    m_titleFormatter.setPattern(settings.value("window_title_format"_L1, u"%if(%p,%p - %t,%t)"_s).toString());

    if(m_update)
    {
        if(ACTION(SkinnedActionManager::WM_ALLWAYS_ON_TOP)->isChecked())
        {
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
            m_playlist->setWindowFlags(m_playlist->windowFlags() | Qt::WindowStaysOnTopHint);
            m_equalizer->setWindowFlags(m_equalizer->windowFlags() | Qt::WindowStaysOnTopHint);
        }
        else
        {
            setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
            m_playlist->setWindowFlags(m_playlist->windowFlags() & ~Qt::WindowStaysOnTopHint);
            m_equalizer->setWindowFlags(m_equalizer->windowFlags() & ~Qt::WindowStaysOnTopHint);
        }
        show();
        qApp->processEvents();
        m_playlist->setVisible(m_display->isPlaylistVisible());
        qApp->processEvents();
        m_equalizer->setVisible(m_display->isEqualizerVisible());

        if(m_pl_manager->currentPlayList()->currentTrack())
            setWindowTitle(m_titleFormatter.format(m_pl_manager->currentPlayList()->currentTrack()));
    }
    else
    {
        QScreen *primaryScreen = QGuiApplication::primaryScreen();
        QRect availableGeometry = primaryScreen->availableGeometry();
        QPoint pos = settings.value("mw_pos"_L1, QPoint(100, 100)).toPoint();
        const QList<QScreen *> screens = QGuiApplication::screens();
        auto it = std::find_if(screens.cbegin(), screens.cend(), [pos](QScreen *screen){ return screen->availableGeometry().contains(pos); });
        if(it != screens.cend())
            availableGeometry = (*it)->availableGeometry();
        pos.setX(qBound(availableGeometry.left(), pos.x(), availableGeometry.right() - m_skin->scaled(275)));
        pos.setY(qBound(availableGeometry.top(), pos.y(), availableGeometry.bottom() - m_skin->scaled(116)));
        move(pos); //geometry
        m_startHidden = settings.value("start_hidden"_L1, false).toBool();
        if(settings.value("always_on_top"_L1, false).toBool())
        {
            ACTION(SkinnedActionManager::WM_ALLWAYS_ON_TOP)->setChecked(true);
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
            m_playlist->setWindowFlags(m_playlist->windowFlags() | Qt::WindowStaysOnTopHint);
            m_equalizer->setWindowFlags(m_equalizer->windowFlags() | Qt::WindowStaysOnTopHint);
        }
        ACTION(SkinnedActionManager::WM_STICKY)->setChecked(settings.value("show_on_all_desktops"_L1,
                                                                    false).toBool());
        show();
        qApp->processEvents();
        //visibility
        m_playlist->setVisible(settings.value("pl_visible"_L1, true).toBool());
        qApp->processEvents();
        m_equalizer->setVisible(settings.value("eq_visible"_L1, true).toBool());
        qApp->processEvents();
        // Repeat/Shuffle
        m_display->setIsRepeatable(m_ui_settings->isRepeatableList());
        m_display->setIsShuffle(m_ui_settings->isShuffle());
        ACTION(SkinnedActionManager::REPEAT_ALL)->setChecked(m_ui_settings->isRepeatableList());
        ACTION(SkinnedActionManager::SHUFFLE)->setChecked(m_ui_settings->isShuffle());
        ACTION(SkinnedActionManager::REPEAT_TRACK)->setChecked(m_ui_settings->isRepeatableTrack());
        ACTION(SkinnedActionManager::NO_PL_ADVANCE)->setChecked(m_ui_settings->isNoPlayListAdvance());
        ACTION(SkinnedActionManager::TRANSIT_BETWEEN_PLAYLISTS)->setChecked(m_ui_settings->isPlayListTransitionEnabled());
        m_update = true;
    }
#ifdef QMMP_WS_X11
    WindowSystem::changeWinSticky(winId(), ACTION(SkinnedActionManager::WM_STICKY)->isChecked());
    WindowSystem::setWinHint(winId(), "player", "xamp");
#endif
    //Call setWindowOpacity only if needed
    double opacity = settings.value("mw_opacity"_L1, 1.0).toDouble();
    if(opacity != windowOpacity())
        setWindowOpacity(opacity);

    opacity = settings.value("eq_opacity"_L1, 1.0).toDouble();
    if(opacity !=  m_equalizer->windowOpacity())
        m_equalizer->setWindowOpacity(opacity);

    opacity = settings.value("pl_opacity"_L1, 1.0).toDouble();
    if(opacity !=  m_playlist->windowOpacity())
        m_playlist->setWindowOpacity(opacity);

    m_hideOnClose = settings.value("hide_on_close"_L1, false).toBool();
    settings.endGroup();

    Dock::instance()->addActions(m_uiHelper->actions(UiHelper::PLAYLIST_MENU));
    Dock::instance()->addActions(m_uiHelper->actions(UiHelper::TOOLS_MENU));
}

void SkinnedMainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    //geometry
    settings.setValue("mw_pos"_L1, this->pos());
    //look & feel
    settings.setValue("double_size"_L1, ACTION(SkinnedActionManager::WM_DOUBLE_SIZE)->isChecked());
    settings.setValue("zoom150"_L1, ACTION(SkinnedActionManager::WM_ZOOM_150)->isChecked());
    settings.setValue("always_on_top"_L1, ACTION(SkinnedActionManager::WM_ALLWAYS_ON_TOP)->isChecked());
    settings.setValue("show_on_all_desktops"_L1, ACTION(SkinnedActionManager::WM_STICKY)->isChecked());
    settings.setValue("antialiasing"_L1, ACTION(SkinnedActionManager::WM_ANTIALIASING)->isChecked());
    settings.endGroup();
}

void SkinnedMainWindow::showSettings()
{
    ConfigDialog *confDialog = new ConfigDialog(this);
    SkinnedSettings *skinnedSettings = new SkinnedSettings(this);
    SkinnedHotkeyEditor *hotkeyEditor = new SkinnedHotkeyEditor(this);
    confDialog->addPage(tr("Appearance"), skinnedSettings, QIcon(u":/skinned/interface.png"_s));
    confDialog->addPage(tr("Shortcuts"), hotkeyEditor, QIcon(u":/skinned/shortcuts.png"_s));
    if(confDialog->exec() == QDialog::Accepted)
    {
        skinnedSettings->writeSettings();
        hotkeyEditor->applyShortcuts();
        updateSettings();
        SkinnedActionManager::instance()->saveActions();
    }
    confDialog->deleteLater();
}

void SkinnedMainWindow::toggleVisibility()
{
    if(isHidden() || isMinimized())
    {
        show();
        raise();
        activateWindow();
        m_playlist->setVisible(m_display->isPlaylistVisible());
        m_equalizer->setVisible(m_display->isEqualizerVisible());
#ifdef QMMP_WS_X11
        if(WindowSystem::netWindowManagerName() == u"Metacity"_s)
        {
            m_playlist->activateWindow();
            m_equalizer->activateWindow();
        }
#endif
        qApp->processEvents();
        setFocus ();
        if(isMinimized())
        {
            showNormal();
        }
#ifdef QMMP_WS_X11
        WindowSystem::changeWinSticky(winId(), ACTION(SkinnedActionManager::WM_STICKY)->isChecked());
        WindowSystem::setWinHint(winId(), "player", "xamp");
        raise();
#endif
    }
    else
    {
        if(m_playlist->isVisible())
            m_playlist->hide();
        if(m_equalizer->isVisible())
            m_equalizer->hide();
        hide();
    }
    qApp->processEvents();
}

void SkinnedMainWindow::showAndRaise()
{
    if(isHidden() || isMinimized())
        toggleVisibility();
    else
    {
        activateWindow();
        raise();
    }
}

void SkinnedMainWindow::createActions()
{
    SET_ACTION(SkinnedActionManager::PL_ADD_FILE, this, &SkinnedMainWindow::addFile);
    SET_ACTION(SkinnedActionManager::PL_ADD_DIRECTORY, this, &SkinnedMainWindow::addDir);
    SET_ACTION(SkinnedActionManager::PL_ADD_URL, this, &SkinnedMainWindow::addUrl);

    m_mainMenu = new QMenu(this);
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::PLAY, m_player, &MediaPlayer::play));
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::PAUSE, m_player, &MediaPlayer::pause));
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::STOP, m_player, &MediaPlayer::stop));
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::PREVIOUS, m_player, &MediaPlayer::previous));
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::NEXT, m_player, &MediaPlayer::next));
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::PLAY_PAUSE, this, &SkinnedMainWindow::playPause));
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::JUMP, this, &SkinnedMainWindow::jumpToTrack));
    m_mainMenu->addSeparator();
    QMenu *viewMenu = m_mainMenu->addMenu(tr("View"));
    viewMenu->addAction(ACTION(SkinnedActionManager::SHOW_PLAYLIST));
    viewMenu->addAction(ACTION(SkinnedActionManager::SHOW_EQUALIZER));
    viewMenu->addSeparator();
    viewMenu->addAction(SET_ACTION(SkinnedActionManager::WM_ALLWAYS_ON_TOP, this, &SkinnedMainWindow::updateSettings));
    viewMenu->addAction(SET_ACTION(SkinnedActionManager::WM_STICKY, this, &SkinnedMainWindow::updateSettings));
    viewMenu->addAction(SET_ACTION(SkinnedActionManager::WM_DOUBLE_SIZE, this, &SkinnedMainWindow::updateSettings));
    viewMenu->addAction(SET_ACTION(SkinnedActionManager::WM_ZOOM_150, this, &SkinnedMainWindow::updateSettings));
    viewMenu->addAction(SET_ACTION(SkinnedActionManager::WM_ANTIALIASING, this, &SkinnedMainWindow::updateSettings));

    QMenu *plMenu = m_mainMenu->addMenu(tr("Playlist"));
    plMenu->addAction(SET_ACTION(SkinnedActionManager::REPEAT_ALL, m_ui_settings, &QmmpUiSettings::setRepeatableList));
    plMenu->addAction(SET_ACTION(SkinnedActionManager::REPEAT_TRACK, m_ui_settings, &QmmpUiSettings::setRepeatableTrack));
    plMenu->addAction(SET_ACTION(SkinnedActionManager::SHUFFLE, m_ui_settings, &QmmpUiSettings::setShuffle));
    plMenu->addAction(SET_ACTION(SkinnedActionManager::NO_PL_ADVANCE, m_ui_settings, &QmmpUiSettings::setNoPlayListAdvance));
    plMenu->addAction(SET_ACTION(SkinnedActionManager::TRANSIT_BETWEEN_PLAYLISTS, m_ui_settings, &QmmpUiSettings::setPlayListTransitionEnabled));
    plMenu->addAction(SET_ACTION(SkinnedActionManager::STOP_AFTER_SELECTED, m_pl_manager, &PlayListManager::stopAfterSelected));
    plMenu->addAction(SET_ACTION(SkinnedActionManager::CLEAR_QUEUE, m_pl_manager, &PlayListManager::clearQueue));
    plMenu->addSeparator();
    plMenu->addAction(ACTION(SkinnedActionManager::PL_SHOW_HEADER));
    plMenu->addAction(ACTION(SkinnedActionManager::PL_SHOW_TABBAR));
    plMenu->addAction(ACTION(SkinnedActionManager::PL_GROUP_TRACKS));

    connect(m_ui_settings, &QmmpUiSettings::repeatableListChanged, ACTION(SkinnedActionManager::REPEAT_ALL), &QAction::setChecked);
    connect(m_ui_settings, &QmmpUiSettings::repeatableTrackChanged, ACTION(SkinnedActionManager::REPEAT_TRACK), &QAction::setChecked);
    connect(m_ui_settings, &QmmpUiSettings::noPlayListAdvanceChanged, ACTION(SkinnedActionManager::NO_PL_ADVANCE), &QAction::setChecked);
    connect(m_ui_settings, &QmmpUiSettings::shuffleChanged, ACTION(SkinnedActionManager::SHUFFLE), &QAction::setChecked);
    connect(m_ui_settings, &QmmpUiSettings::playListTransitionChanged, ACTION(SkinnedActionManager::TRANSIT_BETWEEN_PLAYLISTS), &QAction::setChecked);

    QMenu *audioMenu = m_mainMenu->addMenu(tr("Audio"));
    audioMenu->addAction(SET_ACTION(SkinnedActionManager::VOL_ENC, m_core, &SoundCore::volumeUp));
    audioMenu->addAction(SET_ACTION(SkinnedActionManager::VOL_DEC, m_core, &SoundCore::volumeDown));
    audioMenu->addAction(SET_ACTION(SkinnedActionManager::VOL_MUTE, m_core, &SoundCore::setMuted));
    connect(m_core, &SoundCore::mutedChanged, ACTION(SkinnedActionManager::VOL_MUTE), &QAction::setChecked);

    m_visMenu = new VisualMenu(this);
    m_mainMenu->addMenu(m_visMenu);
    m_mainMenu->addMenu(m_uiHelper->createMenu(UiHelper::TOOLS_MENU, tr("Tools"), true, this));

    m_mainMenu->addSeparator();
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::SETTINGS, this, &SkinnedMainWindow::showSettings));
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::ABOUT, this, &SkinnedMainWindow::about));
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::ABOUT_QT, qApp, &QApplication::aboutQt));
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(SET_ACTION(SkinnedActionManager::QUIT, m_uiHelper, &UiHelper::exit));
    //seeking
    SET_ACTION(SkinnedActionManager::SEEK_FORWARD_10, this, [this] { m_core->seekRelative(10000); } );
    SET_ACTION(SkinnedActionManager::SEEK_FORWARD_30, this, [this] { m_core->seekRelative(30000); } );
    SET_ACTION(SkinnedActionManager::SEEK_FORWARD_60, this, [this] { m_core->seekRelative(60000); } );
    SET_ACTION(SkinnedActionManager::SEEK_BACKWARD_10, this, [this] { m_core->seekRelative(-10000); } );
    SET_ACTION(SkinnedActionManager::SEEK_BACKWARD_30, this, [this] { m_core->seekRelative(-30000); } );
    SET_ACTION(SkinnedActionManager::SEEK_BACKWARD_60, this, [this] { m_core->seekRelative(-60000); } );

    Dock::instance()->addActions(SkinnedActionManager::instance()->actions());
}

void SkinnedMainWindow::about()
{
    m_uiHelper->about(this);
}

void SkinnedMainWindow::updateSettings()
{
    readSettings();
    m_playlist->readSettings();
    m_visMenu->updateActions();
    m_skin->reloadSkin();
    Dock::instance()->updateDock();
}

void SkinnedMainWindow::loadPlaylist()
{
    m_uiHelper->loadPlayList(this);
}

void SkinnedMainWindow::savePlaylist()
{
    m_uiHelper->savePlayList(this);
}

void SkinnedMainWindow::playPause()
{
    if(m_core->state() == Qmmp::Playing)
        m_player->pause();
    else
        m_player->play();
}

void SkinnedMainWindow::jumpToTrack()
{
    m_uiHelper->jumpToTrack(this);
}

void SkinnedMainWindow::addUrl()
{
    m_uiHelper->addUrl(this);
}

void SkinnedMainWindow::keyPressEvent(QKeyEvent *ke)
{
    QKeyEvent event = QKeyEvent(ke->type(), ke->key(),
                                ke->modifiers(), ke->text(),ke->isAutoRepeat(), ke->count());
    QApplication::sendEvent(m_playlist,&event);
}

void SkinnedMainWindow::restoreWindowTitle()
{
    setWindowTitle(tr("x-AMP"));
}
