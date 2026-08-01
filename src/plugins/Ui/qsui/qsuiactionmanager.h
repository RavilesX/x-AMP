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

#ifndef QSUIACTIONMANAGER_H
#define QSUIACTIONMANAGER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <QToolBar>
#include <utility>

class QAction;
class QSettings;
class QDockWidget;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class QSUiActionManager : public QObject
{
    Q_OBJECT
public:
    explicit QSUiActionManager(QObject *parent = nullptr);
    ~QSUiActionManager();

    enum Type
    {
        PLAY = 0,
        PAUSE,
        STOP,
        PREVIOUS,
        NEXT,
        PLAY_PAUSE,
        SEEK_FORWARD_10,
        SEEK_FORWARD_30,
        SEEK_FORWARD_60,
        SEEK_BACKWARD_10,
        SEEK_BACKWARD_30,
        SEEK_BACKWARD_60,
        JUMP,
        EJECT,
        RECORD,
        REPEAT_ALL,
        REPEAT_TRACK,
        SHUFFLE,
        NO_PL_ADVANCE,
        TRANSIT_BETWEEN_PLAYLISTS,
        STOP_AFTER_SELECTED,
        CLEAR_QUEUE,

        WM_ALLWAYS_ON_TOP,
        WM_STICKY,
        UI_ANALYZER,              //external
        UI_FILEBROWSER,           //external
        UI_COVER,                 //external
        UI_PLAYLIST_BROWSER,      //external
        UI_WAVEFORM_SEEKBAR,      //external
        UI_SHOW_TABS,
        UI_BLOCK_DOCKWIDGETS,
        UI_BLOCK_TOOLBARS,
        UI_SHOW_MENUBAR,
        UI_SHOW_STATUSBAR,

        VOL_ENC,
        VOL_DEC,
        VOL_MUTE,

        PL_ADD_FILE,
        PL_ADD_DIRECTORY,
        PL_ADD_URL,
        PL_REMOVE_SELECTED,
        PL_REMOVE_ALL,
        PL_REMOVE_UNSELECTED,
        PL_REMOVE_INVALID,
        PL_REMOVE_DUPLICATES,
        PL_REFRESH,
        PL_ENQUEUE,
        PL_INVERT_SELECTION,
        PL_CLEAR_SELECTION,
        PL_SELECT_ALL,
        PL_SHOW_INFO,
        PL_NEW,
        PL_CLOSE,
        PL_LOAD,
        PL_SAVE,
        PL_RENAME,
        PL_SELECT_NEXT,
        PL_SELECT_PREVIOUS,
        PL_GROUP_TRACKS,
        PL_SHOW_HEADER,

        EQUALIZER,
        SETTINGS,
        APPLICATION_MENU,
        ABOUT_UI,
        ABOUT,
        ABOUT_QT,
        QUIT,

        //widgets
        UI_POS_SLIDER,            //external
        UI_VOL_SLIDER,            //external
        UI_BAL_SLIDER,            //external
        UI_QUICK_SEARCH,          //external
        UI_SEPARATOR,
    };

    struct ToolBarInfo
    {
        QString title;
        QString uid;
        QStringList actionNames;
        QSize iconSize;
    };

    QAction *action(int type);
    QList<QAction *> actions() const;
    QList<QDockWidget *> dockWidgtes() const;
    bool hasDockWidgets() const;
    void saveActions();
    void resetShortcuts();
    void registerAction(int id, QAction *action, const QString &confKey, const QKeySequence &key = QKeySequence());
    void registerWidget(int id, QWidget *w, const QString &text, const QString &name);
    void registerDockWidget(QDockWidget *w, const QString &confKey, const QKeySequence &key);
    void removeDockWidget(QDockWidget *w);
    QToolBar *createToolBar(const ToolBarInfo &info, QWidget *parent);
    void updateToolBar(QToolBar *toolBar, const ToolBarInfo &info);
    QSUiActionManager::ToolBarInfo defaultToolBar() const;
    QList<ToolBarInfo> readToolBarSettings() const;
    void writeToolBarSettings(const QList<ToolBarInfo> &l);

    static QSUiActionManager* instance();

private:
    QAction *createAction(const QString &name, const QString &confKey, const QKeySequence &key = QKeySequence(),
                          const QString &iconName = QString());
    QAction *createAction2(const QString &name, const QString &confKey, const QKeySequence &key = QKeySequence(),
                           const QString &iconName = QString());
    void readStates();
    void saveStates();

    QSettings *m_settings;
    QHash<int, QAction *> m_actions;
    QHash<QDockWidget *, std::pair<QString, QString>> m_dockWidgets; //widget, key, default shortcut
    static QSUiActionManager *m_instance;

};

template <class Obj, typename Func1>
inline QAction *SET_ACTION(int type, const Obj *object, Func1 slot)
{
    QAction *act = QSUiActionManager::instance()->action(type);
    QObject::connect(act, &QAction::triggered, object, slot);
    return act;
}

inline QAction *ACTION(int type)
{
    return QSUiActionManager::instance()->action(type);
}


#endif // QSUIACTIONMANAGER_H
