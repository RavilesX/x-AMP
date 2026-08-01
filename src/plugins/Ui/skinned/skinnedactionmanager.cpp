/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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
#include <QAction>
#include <QIcon>
#include <QFile>
#include <QApplication>
#include <qmmp/qmmp.h>
#include "skinnedactionmanager.h"

SkinnedActionManager *SkinnedActionManager::m_instance = nullptr;

SkinnedActionManager::SkinnedActionManager(QObject *parent) :
    QObject(parent)
{
    m_instance = this;
    m_settings = new QSettings;
    m_settings->beginGroup("SkinnedShortcuts"_L1);
    m_actions = {
        //playback
        { PLAY, createAction(tr("&Play"), u"play"_s, tr("X"), u"media-playback-start"_s) },
        { PAUSE, createAction(tr("&Pause"), u"pause"_s, tr("C"), u"media-playback-pause"_s) },
        { STOP, createAction(tr("&Stop"), u"stop"_s, tr("V"), u"media-playback-stop"_s) },
        { PREVIOUS, createAction(tr("&Previous"), u"previous"_s, tr("Z"), u"media-skip-backward"_s) },
        { NEXT, createAction(tr("&Next"), u"next"_s, tr("B"), u"media-skip-forward"_s) },
        { PLAY_PAUSE, createAction(tr("&Play/Pause"), u"play_pause"_s, Qt::Key_Space ) },
        { SEEK_FORWARD_10, createAction(tr("+10 seconds"), u"seek_forward_10"_s, Qt::Key_Right, u"media-seek-forward"_s) },
        { SEEK_FORWARD_30, createAction(tr("+30 seconds"), u"seek_forward_30"_s, Qt::CTRL | Qt::Key_Right, u"media-seek-forward"_s) },
        { SEEK_FORWARD_60, createAction(tr("+60 seconds"), u"seek_forward_60"_s, QKeySequence(), u"media-seek-forward"_s) },
        { SEEK_BACKWARD_10, createAction(tr("-10 seconds"), u"seek_backward_10"_s, Qt::Key_Left, u"media-seek-backward"_s) },
        { SEEK_BACKWARD_30, createAction(tr("-30 seconds"), u"seek_backward_20"_s, Qt::CTRL | Qt::Key_Left, u"media-seek-backward"_s) },
        { SEEK_BACKWARD_60, createAction(tr("-60 seconds"), u"seek_backward_30"_s, QKeySequence(), u"media-seek-backward"_s) },
        { JUMP, createAction(tr("&Jump to Track"), u"jump"_s, tr("J"), u"go-up"_s) },
        { REPEAT_ALL, createAction2(tr("&Repeat Playlist"), u"repeate_playlist"_s, tr("R")) },
        { REPEAT_TRACK, createAction2(tr("&Repeat Track"), u"repeate_track"_s, tr("Ctrl+R")) },
        { SHUFFLE, createAction2(tr("&Shuffle"), u"shuffle"_s, tr("S")) },
        { NO_PL_ADVANCE, createAction2(tr("&No Playlist Advance"), u"no_playlist_advance"_s, tr("Ctrl+N")) },
        { STOP_AFTER_SELECTED, createAction(tr("&Stop After Selected"), u"stop_after_selected"_s, tr("Ctrl+S")) },
        { TRANSIT_BETWEEN_PLAYLISTS, createAction2(tr("&Transit between playlists"), u"transit_between_playlists"_s) },
        { CLEAR_QUEUE, createAction(tr("&Clear Queue"), u"clear_queue"_s, tr("Alt+Q")) },
        //view
        { SHOW_PLAYLIST, createAction2(tr("Show Playlist"), u"show_playlist"_s, tr("Alt+E")) },
        { SHOW_EQUALIZER, createAction2(tr("Show Equalizer"), u"show_equalizer"_s, tr("Alt+G")) },
        { WM_ALLWAYS_ON_TOP, createAction2(tr("Always on Top"), u"always_on_top"_s) },
        { WM_STICKY, createAction2(tr("Put on All Workspaces"), u"sticky"_s) },
        { WM_DOUBLE_SIZE, createAction2(tr("Double Size"), u"double_size"_s, tr("Meta+D")) },
        { WM_ZOOM_150, createAction2(tr("Zoom 150%"), u"zoom150"_s, tr("Meta+E")) },
        { WM_ANTIALIASING, createAction2(tr("Anti-aliasing"), u"anti_aliasing"_s) },
        //volume
        { VOL_ENC, createAction(tr("Volume &+"), u"vol_enc"_s, tr("0")) },
        { VOL_DEC, createAction(tr("Volume &-"), u"vol_dec"_s, tr("9")) },
        { VOL_MUTE, createAction2(tr("&Mute"), u"vol_mute"_s, tr("M")) },
        //playlist
        { PL_ADD_FILE, createAction(tr("&Add File"), u"add_file"_s, tr("F"), u"audio-x-generic"_s) },
        { PL_ADD_DIRECTORY, createAction(tr("&Add Directory"), u"add_dir"_s, tr("D"), u"folder"_s) },
        { PL_ADD_URL, createAction(tr("&Add Url"), u"add_url"_s, tr("U"), u"network-server"_s) },
        { PL_REMOVE_SELECTED, createAction(tr("&Remove Selected"), u"remove_selected"_s, tr("Del"), u"edit-delete"_s) },
        { PL_REMOVE_ALL, createAction(tr("&Remove All"), u"remove_all"_s, QKeySequence(), u"edit-clear"_s) },
        { PL_REMOVE_UNSELECTED, createAction(tr("&Remove Unselected"), u"remove_unselected"_s, QKeySequence(), u"edit-delete"_s) },
        { PL_REMOVE_INVALID, createAction(tr("Remove unavailable files"), u"remove_invalid"_s, QKeySequence(), u"dialog-error"_s) },
        { PL_REMOVE_DUPLICATES, createAction(tr("Remove duplicates"), u"remove_duplicates"_s) },
        { PL_REFRESH, createAction(tr("Refresh"), u"refresh"_s, u"F5"_s, u"view-refresh"_s) },
        { PL_ENQUEUE, createAction(tr("&Queue Toggle"), u"enqueue"_s, tr("Q")) },
        { PL_INVERT_SELECTION, createAction(tr("Invert Selection"), u"invert_selection"_s) },
        { PL_CLEAR_SELECTION, createAction(tr("&Select None"), u"clear_selection"_s) },
        { PL_SELECT_ALL, createAction(tr("&Select All"), u"select_all"_s, tr("Ctrl+A"), u"edit-select-all"_s) },
        { PL_SHOW_INFO, createAction(tr("&View Track Details"), u"show_info"_s, tr("Alt+I"), u"dialog-information"_s) },
        { PL_NEW, createAction(tr("&New List"), u"new_pl"_s, tr("Ctrl+T"), u"document-new"_s) },
        { PL_CLOSE, createAction(tr("&Delete List"), u"close_pl"_s, tr("Ctrl+W"), u"window-close"_s) },
        { PL_LOAD, createAction(tr("&Load List"), u"load_pl"_s, tr("O"), u"document-open"_s) },
        { PL_SAVE, createAction(tr("&Save List"), u"save_pl"_s, tr("Shift+S"), u"document-save-as"_s) },
        { PL_RENAME, createAction(tr("&Rename List"), u"pl_rename"_s, tr("F2")) },
        { PL_SELECT_NEXT, createAction(tr("&Select Next Playlist"), u"next_pl"_s, tr("Ctrl+PgDown"), u"go-next"_s) },
        { PL_SELECT_PREVIOUS, createAction(tr("&Select Previous Playlist"), u"prev_pl"_s, tr("Ctrl+PgUp"), u"go-previous"_s) },
        { PL_SHOW_MANAGER, createAction(tr("&Show Playlists"), u"show_pl_manager"_s, tr("P"), u"view-list-details"_s) },
        { PL_GROUP_TRACKS, createAction2(tr("&Group Tracks"), u"group_tracks"_s, tr("Ctrl+G")) },
        { PL_SHOW_HEADER, createAction2(tr("&Show Column Headers"), u"show_header"_s, tr("Ctrl+H")) },
        { PL_SHOW_TABBAR, createAction2(tr("Show &Tab Bar"), u"show_pl_tabbar"_s, tr("Alt+T")) },
        //other
        { SETTINGS, createAction(tr("&Settings"), u"show_settings"_s, tr("Ctrl+P"), u"configure"_s) },
        { ABOUT, createAction(tr("&About"), u"about"_s) },
        { ABOUT_QT, createAction(tr("&About Qt"), u"about_qt"_s) },
        { QUIT, createAction(tr("&Exit"), u"exit"_s, tr("Ctrl+Q"), u"application-exit"_s) },
      };
    m_settings->endGroup();
    readStates();
    delete m_settings;
    m_settings = nullptr;
    m_actions[ABOUT]->setIcon(qApp->windowIcon());
    //x-AMP: Double Size and Zoom 150% are mutually exclusive but both may be
    //off; anti-aliasing is meaningful whenever any scaling is active (at 150%
    //smooth scaling is forced regardless, see Skin::scalePixmap)
    auto updateScaleActions = [this] {
        if(sender() == m_actions[WM_DOUBLE_SIZE] && m_actions[WM_DOUBLE_SIZE]->isChecked())
            m_actions[WM_ZOOM_150]->setChecked(false);
        else if(sender() == m_actions[WM_ZOOM_150] && m_actions[WM_ZOOM_150]->isChecked())
            m_actions[WM_DOUBLE_SIZE]->setChecked(false);
        m_actions[WM_ANTIALIASING]->setEnabled(m_actions[WM_DOUBLE_SIZE]->isChecked() ||
                                               m_actions[WM_ZOOM_150]->isChecked());
    };
    connect(m_actions[WM_DOUBLE_SIZE], &QAction::toggled, this, updateScaleActions);
    connect(m_actions[WM_ZOOM_150], &QAction::toggled, this, updateScaleActions);
    m_actions[WM_ANTIALIASING]->setEnabled(false);
}

SkinnedActionManager::~SkinnedActionManager()
{
    saveStates();
    m_instance = nullptr;
}

QAction *SkinnedActionManager::action(int type) const
{
    return m_actions[type];
}

QList<QAction *> SkinnedActionManager::actions() const
{
    return m_actions.values();
}

SkinnedActionManager *SkinnedActionManager::instance()
{
    return m_instance;
}

QAction *SkinnedActionManager::createAction(const QString &name, const QString &confKey, const QKeySequence &key, const QString &iconName)
{
    QAction *action = new QAction(name, this);
    action->setShortcutVisibleInContextMenu(true);
    action->setShortcut(QKeySequence(m_settings->value(confKey, key.toString()).toString(), QKeySequence::PortableText));
    action->setProperty("defaultShortcut", key);
    action->setObjectName(confKey);
    if(iconName.isEmpty())
        return action;
    if(QFile::exists(iconName))
        action->setIcon(QIcon(iconName));
    else
        action->setIcon(QIcon::fromTheme(iconName));
    return action;
}

QAction *SkinnedActionManager::createAction2(const QString &name, const QString &confKey, const QKeySequence &key)
{
    QAction *action = createAction(name, confKey, key);
    action->setCheckable(true);
    return action;
}

void SkinnedActionManager::readStates()
{
    m_settings->beginGroup("Skinned"_L1);
    m_actions[PL_SHOW_HEADER]->setChecked(m_settings->value("pl_show_header"_L1, false).toBool());
    m_actions[PL_SHOW_TABBAR]->setChecked(m_settings->value("pl_show_tabbar"_L1, false).toBool());
    m_settings->endGroup();
}

void SkinnedActionManager::saveStates()
{
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    settings.setValue("pl_show_header"_L1, m_actions[PL_SHOW_HEADER]->isChecked());
    settings.setValue("pl_show_tabbar"_L1, m_actions[PL_SHOW_TABBAR]->isChecked());
    settings.endGroup();
}

void SkinnedActionManager::saveActions()
{
    QSettings settings;
    for(const QAction *action : std::as_const(m_actions))
    {
        settings.setValue(QStringLiteral("SkinnedShortcuts/") + action->objectName(), action->shortcut().toString());
    }
}

void SkinnedActionManager::resetShortcuts()
{
    for(QAction *action : std::as_const(m_actions))
    {
        action->setShortcut(action->property("defaultShortcut").value<QKeySequence>());
    }
}
