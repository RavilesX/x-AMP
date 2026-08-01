/***************************************************************************
 *   Copyright (C) 2017-2026 by Ilya Kotov                                 *
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

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QAction>
#include <QApplication>
#include <qmmpui/uihelper.h>
#include <qmmp/soundcore.h>
#include "historywindow.h"
#include "history.h"

History::History(QObject *parent) : QObject(parent)
{
    m_core = SoundCore::instance();
    connect(m_core, &SoundCore::trackInfoChanged, this, &History::onTrackInfoChanged);
    connect(m_core, &SoundCore::stateChanged, this, &History::onStateChanged);

    QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, CONNECTION_NAME);
    if(db.isValid() && !db.isOpen())
    {
        db.setDatabaseName(Qmmp::configDir() + u"/history.sqlite"_s);
        db.open();
        if(createTables())
        {
            QSqlQuery query(db);
            query.exec(u"PRAGMA journal_mode = WAL"_s);
            query.exec(u"PRAGMA synchronous = NORMAL"_s);
            qCDebug(plugin, "database initialization finished");
        }
        else
        {
            db.close();
            qCWarning(plugin, "plugin is disabled");
        }
    }

    QAction *action = new QAction(tr("History"), this);
    action->setShortcut(tr("Alt+H"));
    action->setIcon(QIcon::fromTheme(u"text-x-generic"_s));
    UiHelper::instance()->addAction(action, UiHelper::TOOLS_MENU);
    connect(action, &QAction::triggered, this, &History::showHistoryWindow);
}

History::~History()
{
    if(QSqlDatabase::contains(CONNECTION_NAME))
    {
        QSqlDatabase::database(CONNECTION_NAME).close();
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
}

void History::onTrackInfoChanged()
{
    if(m_elapsed + m_time.elapsed() > 20000)
        saveTrack();

    m_trackInfo = m_core->trackInfo();
    m_time.restart();
    m_elapsed = 0;
}

void History::onStateChanged(Qmmp::State state)
{
    if(state == Qmmp::Playing && m_previousState == Qmmp::Stopped)
    {
        m_time.restart();
    }
    else if(state == Qmmp::Paused)
    {
        m_elapsed += m_time.elapsed();
    }
    else if(state == Qmmp::Stopped)
    {
        if(m_previousState == Qmmp::Playing)
            m_elapsed += m_time.elapsed();

        if(m_elapsed > 20000)
            saveTrack();
        m_elapsed = 0;
    }

    m_previousState = state;
}

void History::showHistoryWindow()
{
    if(!m_historyWindow)
        m_historyWindow = new HistoryWindow(QSqlDatabase::database(CONNECTION_NAME), qApp->activeWindow());
    m_historyWindow->show();
    m_historyWindow->activateWindow();
}

bool History::createTables()
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
        return false;

    QSqlQuery query(db);
    bool ok = query.exec(u"CREATE TABLE IF NOT EXISTS track_history(ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "Timestamp TIMESTAMP NOT NULL,"
                         "Title TEXT, Artist TEXT, AlbumArtist TEXT, Album TEXT, Comment TEXT, Genre TEXT, Composer TEXT,"
                         "Year INTEGER, Track INTEGER, DiscNumber TEXT, Duration INTEGER, URL BLOB)"_s);

    if(!ok)
        qCWarning(plugin, "unable to create table, error: %s", qPrintable(query.lastError().text()));

    return ok;
}

void History::saveTrack()
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen() || m_trackInfo.isEmpty())
        return;

    QSqlQuery query(db);
    query.prepare(u"INSERT INTO track_history VALUES(NULL, CURRENT_TIMESTAMP, :title, :artist, :albumartist, :album, :comment,"
                  ":genre, :composer, :year, :track, :discnumber, :duration, :url);"_s);
    query.bindValue(u":title"_s, m_trackInfo.value(Qmmp::TITLE));
    query.bindValue(u":artist"_s, m_trackInfo.value(Qmmp::ARTIST));
    query.bindValue(u":albumartist"_s, m_trackInfo.value(Qmmp::ALBUMARTIST));
    query.bindValue(u":album"_s, m_trackInfo.value(Qmmp::ALBUM));
    query.bindValue(u":comment"_s, m_trackInfo.value(Qmmp::COMMENT));
    query.bindValue(u":genre"_s, m_trackInfo.value(Qmmp::GENRE));
    query.bindValue(u":composer"_s, m_trackInfo.value(Qmmp::COMPOSER));
    query.bindValue(u":year"_s, m_trackInfo.value(Qmmp::YEAR));
    query.bindValue(u":track"_s, m_trackInfo.value(Qmmp::TRACK));
    query.bindValue(u":discnumber"_s, m_trackInfo.value(Qmmp::DISCNUMBER));
    query.bindValue(u":duration"_s, m_trackInfo.duration());
    query.bindValue(u":url"_s, m_trackInfo.path());
    bool ok = query.exec();

    if(!ok)
        qCWarning(plugin, "unable to save track, error: %s", qPrintable(query.lastError().text()));
    else
        qCDebug(plugin, "track '%s' has been added to history",
               qPrintable(m_trackInfo.value(Qmmp::ARTIST) + u" - "_s + m_trackInfo.value(Qmmp::TITLE)));

    m_trackInfo.clear();
}
