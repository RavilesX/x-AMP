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
#include <QStyle>
#include <QWidgetAction>
#include <QDockWidget>
#include <qmmp/qmmp.h>
#include "qsuiactionmanager.h"

QSUiActionManager *QSUiActionManager::m_instance = nullptr;

QSUiActionManager::QSUiActionManager(QObject *parent) :
    QObject(parent)
{
    m_instance = this;
    m_settings = new QSettings;
    m_settings->beginGroup(u"SimpleUiShortcuts"_s);

    m_actions = {
        //playback
        { PLAY, createAction(tr("&Play"), u"play"_s, tr("X"), u"media-playback-start"_s) },
        { PAUSE, createAction(tr("&Pause"), u"pause"_s, tr("C"), u"media-playback-pause"_s) },
        { STOP, createAction(tr("&Stop"), u"stop"_s, tr("V"), u"media-playback-stop"_s) },
        { PREVIOUS, createAction(tr("&Previous"), u"previous"_s, tr("Z"), u"media-skip-backward"_s) },
        { NEXT, createAction(tr("&Next"), u"next"_s, tr("B"), u"media-skip-forward"_s) },
        { PLAY_PAUSE, createAction(tr("&Play/Pause"), u"play_pause"_s, Qt::Key_Space, u"media-playback-start"_s) },
        { SEEK_FORWARD_10, createAction(tr("+10 seconds"), u"seek_forward_10"_s, Qt::Key_Right, u"media-seek-forward"_s) },
        { SEEK_FORWARD_30, createAction(tr("+30 seconds"), u"seek_forward_30"_s, Qt::CTRL | Qt::Key_Right, u"media-seek-forward"_s) },
        { SEEK_FORWARD_60, createAction(tr("+60 seconds"), u"seek_forward_60"_s, QKeySequence(), u"media-seek-forward"_s) },
        { SEEK_BACKWARD_10, createAction(tr("-10 seconds"), u"seek_backward_10"_s, Qt::Key_Left, u"media-seek-backward"_s) },
        { SEEK_BACKWARD_30, createAction(tr("-30 seconds"), u"seek_backward_20"_s, Qt::CTRL | Qt::Key_Left, u"media-seek-backward"_s) },
        { SEEK_BACKWARD_60, createAction(tr("-60 seconds"), u"seek_backward_30"_s, QKeySequence(), u"media-seek-backward"_s) },
        { JUMP, createAction(tr("&Jump to Track"), u"jump"_s, tr("J"), u"go-up"_s) },
        { EJECT, createAction(tr("&Play Files"), u"eject"_s, tr("E"), u"media-eject"_s) },
        { RECORD, createAction2(tr("&Record"), u"record"_s, QKeySequence(), u"media-record"_s) },
        { REPEAT_ALL, createAction2(tr("&Repeat Playlist"), u"repeate_playlist"_s, tr("R"), u"media-playlist-repeat"_s) },
        { REPEAT_TRACK, createAction2(tr("&Repeat Track"), u"repeate_track"_s, tr("Ctrl+R"), u"media-repeat-single"_s) },
        { SHUFFLE, createAction2(tr("&Shuffle"), u"shuffle"_s, tr("S"), u"media-playlist-shuffle"_s) },
        { NO_PL_ADVANCE, createAction2(tr("&No Playlist Advance"), u"no_playlist_advance"_s, tr("Ctrl+N")) },
        { TRANSIT_BETWEEN_PLAYLISTS, createAction2(tr("&Transit between playlists"), u"transit_between_playlists"_s) },
        { STOP_AFTER_SELECTED, createAction(tr("&Stop After Selected"), u"stop_after_selected"_s, tr("Ctrl+S")) },
        { CLEAR_QUEUE, createAction(tr("&Clear Queue"), u"clear_queue"_s, tr("Alt+Q")) },
        //view
        { WM_ALLWAYS_ON_TOP, createAction2(tr("Always on Top"), u"always_on_top"_s) },
        { WM_STICKY, createAction2(tr("Put on All Workspaces"), u"sticky"_s) },
        { UI_ANALYZER, nullptr }, //external action
        { UI_FILEBROWSER, nullptr }, //external action
        { UI_COVER, nullptr }, //external action
        { UI_PLAYLIST_BROWSER, nullptr }, //external action
        { UI_WAVEFORM_SEEKBAR, nullptr }, //external action
        { UI_SHOW_TABS, createAction2(tr("Show Tabs"), u"show_tabs"_s) },
        { UI_BLOCK_DOCKWIDGETS, createAction2(tr("Block Floating Panels"), u"block_dockwidgets"_s) },
        { UI_BLOCK_TOOLBARS, createAction2(tr("Block Toolbars"), u"block_toolbars"_s) },
        { UI_SHOW_MENUBAR, createAction2(tr("Menu Bar"), u"show_menubar"_s) },
        { UI_SHOW_STATUSBAR, createAction2(tr("Status Bar"), u"show_statusbar"_s) },

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
        { PL_GROUP_TRACKS, createAction2(tr("&Group Tracks"), u"group_tracks"_s, tr("Ctrl+G")) },
        { PL_SHOW_HEADER, createAction2(tr("&Show Column Headers"), u"show_header"_s, tr("Ctrl+H")) },
        //other
        { EQUALIZER, createAction(tr("&Equalizer"), u"equalizer"_s, tr("Ctrl+E")) },
        { SETTINGS, createAction(tr("&Settings"), u"show_settings"_s, tr("Ctrl+P"), u"configure"_s) },
        { APPLICATION_MENU, createAction(tr("Application Menu"), u"app_menu"_s, QKeySequence(), u"format-justify-fill"_s) },
        { ABOUT_UI, createAction(tr("&About Ui"), u"about_ui"_s) },
        { ABOUT, createAction(tr("&About"), u"about"_s) },
        { ABOUT_QT, createAction(tr("&About Qt"), u"about_qt"_s) },
        { QUIT, createAction(tr("&Exit"), u"exit"_s, tr("Ctrl+Q"), u"application-exit"_s) }
    };
    m_settings->endGroup();
    readStates();
    delete m_settings;
    m_settings = nullptr;
    m_actions[ABOUT]->setIcon(qApp->windowIcon());
}

QSUiActionManager::~QSUiActionManager()
{
    saveStates();
    m_instance = nullptr;
}

QAction *QSUiActionManager::action(int type)
{
    return m_actions[type];
}

QList<QAction *> QSUiActionManager::actions() const
{
    return m_actions.values();
}

QList<QDockWidget *> QSUiActionManager::dockWidgtes() const
{
    return m_dockWidgets.keys();
}

bool QSUiActionManager::hasDockWidgets() const
{
    return !m_dockWidgets.isEmpty();
}

QSUiActionManager* QSUiActionManager::instance()
{
    return m_instance;
}

QAction *QSUiActionManager::createAction(const QString &name, const QString &confKey, const QKeySequence &key, const QString &iconName)
{
    QAction *action = new QAction(name, this);
    action->setShortcutVisibleInContextMenu(true);
    action->setShortcut(QKeySequence(m_settings->value(confKey, key.toString()).toString(), QKeySequence::PortableText));
    action->setObjectName(confKey);
    action->setProperty("defaultShortcut", key);
    if(iconName.isEmpty())
        return action;
    if(QFile::exists(iconName))
        action->setIcon(QIcon(iconName));
    else if(QIcon::hasThemeIcon(iconName))
        action->setIcon(QIcon::fromTheme(iconName));
    else if(QFile::exists(QStringLiteral(":/qsui/%1.png").arg(iconName)))
        action->setIcon(QIcon(QStringLiteral(":/qsui/%1.png").arg(iconName)));
    return action;
}

QAction *QSUiActionManager::createAction2(const QString &name, const QString &confKey, const QKeySequence &key, const QString &iconName)
{
    QAction *action = createAction(name, confKey, key, iconName);
    action->setCheckable(true);
    action->setIconVisibleInMenu(false);
    return action;
}

void QSUiActionManager::readStates()
{
    m_settings->beginGroup(u"Simple"_s);
    m_actions[PL_SHOW_HEADER]->setChecked(m_settings->value(u"pl_show_header"_s, true).toBool());
    m_settings->endGroup();
}

void QSUiActionManager::saveStates()
{
    QSettings settings;
    settings.beginGroup(u"Simple"_s);
    settings.setValue(u"pl_show_header"_s, m_actions[PL_SHOW_HEADER]->isChecked());
    settings.endGroup();
}

void QSUiActionManager::saveActions()
{
    QSettings settings;
    settings.beginGroup(u"SimpleUiShortcuts"_s);

    for(const QAction *action : std::as_const(m_actions))
    {
        settings.setValue(action->objectName(), action->shortcut().toString());
    }

    auto it = m_dockWidgets.cbegin();
    while(it != m_dockWidgets.cend())
    {
        settings.setValue(it.value().first, it.key()->toggleViewAction()->shortcut().toString());
        ++it;
    }

    settings.endGroup();
}

void QSUiActionManager::resetShortcuts()
{
    for(QAction *action : std::as_const(m_actions))
    {
        action->setShortcut(action->property("defaultShortcut").value<QKeySequence>());
    }

    auto it = m_dockWidgets.cbegin();
    while(it != m_dockWidgets.cend())
    {
        it.key()->toggleViewAction()->setShortcut(it.value().second);
        ++it;
    }
}

void QSUiActionManager::registerAction(int id, QAction *action, const QString &confKey, const QKeySequence &key)
{
    if(m_actions.value(id))
        qCFatal(plugin) << "invalid action id";

    QSettings settings;
    settings.beginGroup(u"SimpleUiShortcuts"_s);
    action->setShortcut(QKeySequence(settings.value(confKey, key.toString()).toString(), QKeySequence::PortableText));
    action->setProperty("defaultShortcut", key);
    action->setObjectName(confKey);
    action->setShortcutVisibleInContextMenu(true);
    m_actions[id] = action;
    settings.endGroup();
}

void QSUiActionManager::registerWidget(int id, QWidget *w, const QString &text, const QString &name)
{
    if(m_actions.value(id))
        qCFatal(plugin) << "invalid action id";
    QWidgetAction *action = new QWidgetAction(this);
    action->setText(text);
    action->setObjectName(name);
    action->setDefaultWidget(w);
    w->setWindowTitle(text);
    m_actions[id] = action;
}

void QSUiActionManager::registerDockWidget(QDockWidget *w, const QString &confKey, const QKeySequence &key)
{
    QSettings settings;
    settings.beginGroup(u"SimpleUiShortcuts"_s);
    w->toggleViewAction()->setShortcut(QKeySequence(settings.value(confKey, key.toString()).toString(), QKeySequence::PortableText));
    settings.endGroup();
    m_dockWidgets.insert(w, std::make_pair(confKey, key.toString()));
}

void QSUiActionManager::removeDockWidget(QDockWidget *w)
{
    m_dockWidgets.remove(w);
}

QToolBar *QSUiActionManager::createToolBar(const ToolBarInfo &info, QWidget *parent)
{
    QToolBar *toolBar = new QToolBar(info.title, parent);
    updateToolBar(toolBar, info);
    toolBar->setProperty("uid", info.uid);
    toolBar->setIconSize(info.iconSize);
    toolBar->setObjectName(u"Toolbar"_s + info.uid);
    return toolBar;
}

void QSUiActionManager::updateToolBar(QToolBar *toolBar, const ToolBarInfo &info)
{
    toolBar->clear();
    toolBar->setIconSize(info.iconSize);
    toolBar->setWindowTitle(info.title);
    for(const QString &name : std::as_const(info.actionNames))
    {
        if(name == QLatin1String("separator"))
        {
            toolBar->addSeparator()->setObjectName(u"separator"_s);
            continue;
        }
        QAction *action = findChild<QAction *>(name);
        if(action)
        {
            action->setVisible(true);
            toolBar->addAction(action);
        }
    }
}

QSUiActionManager::ToolBarInfo QSUiActionManager::defaultToolBar() const
{
    static const QList<Type> idList = {
        PL_ADD_FILE, PL_ADD_DIRECTORY, PREVIOUS, PLAY, PAUSE, STOP, NEXT, EJECT,
        UI_SEPARATOR, UI_POS_SLIDER, UI_SEPARATOR, UI_VOL_SLIDER, VOL_MUTE
    };
    QStringList names;
    for(const Type &id : std::as_const(idList))
    {
        if(id == UI_SEPARATOR)
        {
            names << u"separator"_s;
            continue;
        }
        names << m_actions.value(id)->objectName();
    }
    QSUiActionManager::ToolBarInfo info;
    info.title = tr("Toolbar");
    info.actionNames = names;
    info.uid = u"{68363a0b-f2cd-462a-87ca-e3089db21561}"_s;
    return info;
}

QList<QSUiActionManager::ToolBarInfo> QSUiActionManager::readToolBarSettings() const
{
    QList<ToolBarInfo> list;
    QSettings settings;
    int iconSize = settings.value(u"Simple/toolbar_icon_size"_s, -1).toInt();
    if(iconSize <= 0)
        iconSize = qApp->style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    int size = settings.beginReadArray(u"SimpleUiToolbars"_s);
    for(int i = 0; i < size; ++i)
    {
        ToolBarInfo info;
        settings.setArrayIndex(i);
        info.title = settings.value(u"title"_s).toString();
        info.actionNames = settings.value(u"actions"_s).toStringList();
        info.uid = settings.value(u"uid"_s).toString();
        info.iconSize = QSize(iconSize, iconSize);
        list.append(info);
    }
    settings.endArray();
    if(list.isEmpty())
    {
        list << defaultToolBar();
        list.last().iconSize = QSize(iconSize, iconSize);
    }
    return list;
}

void QSUiActionManager::writeToolBarSettings(const QList<ToolBarInfo> &l)
{
    QSettings settings;
    settings.beginWriteArray(u"SimpleUiToolbars"_s);
    for(int i = 0; i < l.size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue(u"title"_s, l[i].title);
        settings.setValue(u"actions"_s, l[i].actionNames);
        settings.setValue(u"uid"_s, l[i].uid);
    }
    settings.endArray();
}
