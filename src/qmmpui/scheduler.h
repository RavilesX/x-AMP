/***************************************************************************
 *   Copyright (C) 2026 by x-AMP developers                                *
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
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <QObject>
#include <QDateTime>
#include <QTime>
#include <QTimer>
#include "qmmpui_export.h"

class PlayListModel;

/*! @brief The Scheduler class runs a single action when the configured event happens.
 *
 * The scheduler is a one-shot: once the action has been executed it disarms itself,
 * so it never fires twice without the user arming it again.
 */
class QMMPUI_EXPORT Scheduler : public QObject
{
    Q_OBJECT
public:
    /*!
     * Event that fires the action.
     */
    enum Trigger
    {
        AT_TIME = 0,    /*!< At the given time of the day */
        AFTER_INTERVAL, /*!< Once the given amount of time has elapsed */
        PLAYLIST_END    /*!< When the current playlist reaches its end ("Repeat All" is ignored) */
    };
    Q_ENUM(Trigger)
    /*!
     * Action to execute.
     */
    enum Action
    {
        PLAY_FILE = 0,  /*!< Plays a single file */
        PLAY_PLAYLIST,  /*!< Plays a playlist from its first track */
        QUIT,           /*!< Closes the player */
        SUSPEND,        /*!< Suspends the computer */
        SHUTDOWN        /*!< Shuts the computer down */
    };
    Q_ENUM(Action)
    /*!
     * Object constructor. Only one instance is allowed.
     * \param parent Parent object.
     */
    explicit Scheduler(QObject *parent = nullptr);
    /*!
     * Destructor.
     */
    ~Scheduler();
    /*!
     * Returns a pointer to the object's instance.
     */
    static Scheduler *instance();
    /*!
     * Returns \b true if the scheduler is armed, otherwise returns \b false.
     */
    bool isEnabled() const;
    /*!
     * Returns the configured trigger.
     */
    Trigger trigger() const;
    /*!
     * Returns the time of the day used by the \b AT_TIME trigger.
     */
    QTime time() const;
    /*!
     * Returns the amount of minutes used by the \b AFTER_INTERVAL trigger.
     */
    int interval() const;
    /*!
     * Returns the configured action.
     */
    Action action() const;
    /*!
     * Returns the file to play, used by the \b PLAY_FILE action.
     */
    QString filePath() const;
    /*!
     * Returns the name of the playlist to play, used by the \b PLAY_PLAYLIST action.
     */
    QString playListName() const;
    /*!
     * Returns the moment the action is going to be executed, or an invalid value
     * when the scheduler is disabled or waits for the end of the playlist.
     */
    QDateTime deadline() const;
    /*!
     * Returns \b true if the scheduler waits for the end of the current playlist,
     * otherwise returns \b false.
     */
    bool isArmedForPlayListEnd() const;
    /*!
     * Changes all settings at once, saves them and restarts the countdown.
     * \param enabled Arms the scheduler.
     * \param trigger Event that fires the action.
     * \param time Time of the day used by the \b AT_TIME trigger.
     * \param interval Amount of minutes used by the \b AFTER_INTERVAL trigger.
     * \param action Action to execute.
     * \param filePath File used by the \b PLAY_FILE action.
     * \param playListName Playlist used by the \b PLAY_PLAYLIST action.
     */
    void setSettings(bool enabled, Trigger trigger, const QTime &time, int interval,
                     Action action, const QString &filePath, const QString &playListName);
    /*!
     * Arms or disarms the scheduler keeping the remaining settings untouched.
     */
    void setEnabled(bool enabled);
    /*!
     * Executes the configured action right away and disarms the scheduler.
     */
    void execute();

signals:
    /*!
     * Emitted when the scheduler has been armed, disarmed or reconfigured.
     */
    void settingsChanged();
    /*!
     * Emitted right before the action is executed.
     * \param action Action being executed.
     */
    void triggered(Scheduler::Action action);

private:
    void readSettings();
    void writeSettings();
    void rearm();
    void scheduleTick();
    void onTimeout();
    void playFile();
    void playPlayList();
    bool playPath(PlayListModel *model, const QString &path);
    void quitPlayer();
    void powerRequest(bool suspend);

    bool m_enabled = false;
    Trigger m_trigger = AT_TIME;
    QTime m_time = QTime(23, 0);
    int m_interval = 60;
    Action m_action = QUIT;
    QString m_filePath;
    QString m_playListName;
    QDateTime m_deadline;
    QTimer m_timer;
    static Scheduler *m_instance;
};

#endif
