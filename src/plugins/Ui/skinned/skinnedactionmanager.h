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

#ifndef SKINNEDACTIONMANAGER_H
#define SKINNEDACTIONMANAGER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QAction>

class QAction;
class QSettings;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedActionManager : public QObject
{
    Q_OBJECT
public:
    explicit SkinnedActionManager(QObject *parent = nullptr);
    ~SkinnedActionManager();

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

        REPEAT_ALL,
        REPEAT_TRACK,
        SHUFFLE,
        NO_PL_ADVANCE,
        TRANSIT_BETWEEN_PLAYLISTS,
        STOP_AFTER_SELECTED,
        CLEAR_QUEUE,

        SHOW_PLAYLIST,
        SHOW_EQUALIZER,

        WM_ALLWAYS_ON_TOP,
        WM_STICKY,
        WM_DOUBLE_SIZE,
        WM_ANTIALIASING,

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
        PL_SHOW_MANAGER,
        PL_GROUP_TRACKS,
        PL_SHOW_HEADER,
        PL_SHOW_TABBAR,

        SETTINGS,
        ABOUT,
        ABOUT_QT,
        QUIT
    };

    QAction *action(int type) const;
    QList<QAction *> actions() const;
    void saveActions();
    void resetShortcuts();
    static SkinnedActionManager *instance();

private:
    QAction *createAction(const QString &name, const QString &confKey, const QKeySequence &key = QKeySequence(),
                          const QString &iconName = QString());
    QAction *createAction2(const QString &name, const QString &confKey, const QKeySequence &key = QKeySequence());
    void readStates();
    void saveStates();

    QSettings *m_settings;
    QHash <int, QAction *> m_actions;
    static SkinnedActionManager *m_instance;

};

template <class Obj, typename Func1>
inline QAction *SET_ACTION(int type, const Obj *object, Func1 slot)
{
    QAction *act = SkinnedActionManager::instance()->action(type);
    QObject::connect(act, &QAction::triggered, object, slot);
    return act;
}

inline QAction *ACTION(int type)
{
    return SkinnedActionManager::instance()->action(type);
}

#endif // SKINNEDACTIONMANAGER_H
