/***************************************************************************
 *   Copyright (C) 2020-2026 by Ilya Kotov                                 *
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
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <qmmp/qmmp.h>
#include <qmmp/metadatamanager.h>
#include <qmmpui/uihelper.h>
#include "librarymodel.h"
#include "librarywidget.h"
#include "library.h"

#define CONNECTION_NAME u"qmmp_library"_s

Library::Library(QPointer<LibraryWidget> *libraryWidget, QObject *parent) :
    QThread(parent),
    m_libraryWidget(libraryWidget)
{
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, CONNECTION_NAME);
        if(db.isValid() && !db.isOpen())
        {
            db.setDatabaseName(Qmmp::configDir() + u"/library.sqlite"_s);
            db.open();
            if(createTables())
                qCDebug(plugin, "database initialization finished");
            else
                qCWarning(plugin, "unable to create table");
        }
    }
    QSqlDatabase::removeDatabase(CONNECTION_NAME);

    QSettings settings;
    m_dirs = settings.value(u"Library/dirs"_s).toStringList();

    m_showAction = new QAction(QIcon::fromTheme(u"text-x-generic"_s), tr("Library"), this);
    m_showAction->setShortcut(tr("Alt+L"));
    UiHelper::instance()->addAction(m_showAction, UiHelper::TOOLS_MENU);
    connect(m_showAction, &QAction::triggered, this, &Library::showLibraryWindow);
    if(!m_libraryWidget->isNull() && !m_libraryWidget->data()->isWindow())
        m_showAction->setVisible(false);

    QAction *refreshAction = new QAction(QIcon::fromTheme(u"view-refresh"_s), tr("Update library"), this);
    UiHelper::instance()->addAction(refreshAction, UiHelper::TOOLS_MENU);
    connect(refreshAction, &QAction::triggered, this, &Library::startDirectoryScanning);

    connect(this, &QThread::finished, this, [=] {
        if(!m_libraryWidget->isNull())
        {
            m_libraryWidget->data()->setBusyMode(false);
            m_libraryWidget->data()->refresh();
        }
    }, Qt::QueuedConnection);

    if(settings.value(u"Library/recreate_db"_s, false).toBool())
    {
        settings.setValue(u"Library/recreate_db"_s, false);
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, CONNECTION_NAME);
            db.setDatabaseName(Qmmp::configDir() + u"/library.sqlite"_s);
            db.open();
            QSqlQuery query(db);
            query.exec(u"DELETE FROM track_library"_s);
            query.exec(u"REINDEX track_library"_s);
            query.exec(u"VACUUM"_s);
            db.close();
        }
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
        startDirectoryScanning();
    }
}

Library::~Library()
{
    if(isRunning())
    {
        m_stopped = true;
        wait();
    }

    if(QSqlDatabase::contains(CONNECTION_NAME))
    {
        QSqlDatabase::database(CONNECTION_NAME).close();
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
}

QAction *Library::showAction() const
{
    return m_showAction;
}

void Library::showLibraryWindow()
{
    if(m_libraryWidget->isNull())
        *m_libraryWidget = new LibraryWidget(true, qApp->activeWindow());

    if(m_libraryWidget->data()->isWindow())
        m_libraryWidget->data()->show();

    if(isRunning())
        m_libraryWidget->data()->setBusyMode(true);
}

void Library::startDirectoryScanning()
{
    if(isRunning())
        return;

    MetaDataManager::instance()->prepareForAnotherThread();
    m_filters = MetaDataManager::instance()->nameFilters();
    start(QThread::IdlePriority);
    if(!m_libraryWidget->isNull())
        m_libraryWidget->data()->setBusyMode(true);
}

void Library::run()
{
    scanDirectories(m_dirs);
}

bool Library::createTables()
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
        return false;

    QSqlQuery query(db);
    bool ok = query.exec(u"CREATE TABLE IF NOT EXISTS track_library("
                         "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "Timestamp TIMESTAMP NOT NULL,"
                         "Title TEXT, Artist TEXT, AlbumArtist TEXT, Album TEXT, Comment TEXT, Genre TEXT, Composer TEXT,"
                         "Year INTEGER, Track INTEGER, DiscNumber TEXT, Duration INTEGER, "
                         "AudioInfo BLOB, URL TEXT, FilePath TEXT, SearchString TEXT)"_s);

    if(!ok)
    {
        qCWarning(plugin, "unable to create table, error: %s", qPrintable(query.lastError().text()));
        return false;
    }

    ok = query.exec(u"CREATE TABLE IF NOT EXISTS ignored_files(ID INTEGER PRIMARY KEY AUTOINCREMENT, FilePath TEXT UNIQUE)"_s);

    if(!ok)
        qCWarning(plugin, "unable to create ignored file list, error: %s", qPrintable(query.lastError().text()));

    return ok;
}

void Library::addTrack(const TrackInfo &track, const QString &filePath)
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
        return;

    QSqlQuery query(db);
    query.prepare(u"INSERT OR REPLACE INTO track_library VALUES("
                  "(SELECT ID FROM track_library WHERE URL = :url), "
                  ":timestamp, "
                  ":title, :artist, :albumartist, :album, :comment, :genre, :composer, "
                  ":year, :track, :discnumber, :duration, "
                  ":audioinfo, :url, :filepath, :searchstring)"_s);

    QString title = track.value(Qmmp::TITLE).isEmpty() ? track.path().section(QLatin1Char('/'), -1) : track.value(Qmmp::TITLE);
    QString album = track.value(Qmmp::ALBUM).isEmpty() ? tr("Unknown") : track.value(Qmmp::ALBUM);
    QString artist = track.value(Qmmp::ARTIST).isEmpty() ? tr("Unknown") : track.value(Qmmp::ARTIST);

    query.bindValue(u":timestamp"_s, QFileInfo(filePath).lastModified());
    query.bindValue(u":title"_s, title);
    query.bindValue(u":artist"_s, artist);
    query.bindValue(u":albumartist"_s, track.value(Qmmp::ALBUMARTIST));
    query.bindValue(u":album"_s, album);
    query.bindValue(u":comment"_s, track.value(Qmmp::COMMENT));
    query.bindValue(u":genre"_s, track.value(Qmmp::GENRE));
    query.bindValue(u":composer"_s, track.value(Qmmp::COMPOSER));
    query.bindValue(u":year"_s, track.value(Qmmp::YEAR));
    query.bindValue(u":track"_s, track.value(Qmmp::TRACK));
    query.bindValue(u":discnumber"_s, track.value(Qmmp::DISCNUMBER));
    query.bindValue(u":duration"_s, track.duration());
    query.bindValue(u":audioinfo"_s, serializeAudioInfo(track.properties()));
    query.bindValue(u":url"_s, track.path());
    query.bindValue(u":filepath"_s, filePath);
    query.bindValue(u":searchstring"_s, QStringLiteral("%1|||%2|||%3").arg(artist, album, title).toLower());
    if(!query.exec())
        qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
}

QByteArray Library::serializeAudioInfo(const QMap<Qmmp::TrackProperty, QString> &properties)
{
    QJsonObject obj;
    QMap<Qmmp::TrackProperty, QString>::const_iterator it = properties.cbegin();
    while(it != properties.cend())
    {
        QString value = properties[it.key()];

        switch(it.key())
        {
        case Qmmp::BITRATE:
            obj.insert(u"bitrate"_s, value.toInt());
            break;
        case Qmmp::SAMPLERATE:
            obj.insert(u"samplerate"_s, value.toInt());
            break;
        case Qmmp::CHANNELS:
            obj.insert(u"channels"_s, value.toInt());
            break;
        case Qmmp::BITS_PER_SAMPLE:
            obj.insert(u"bitsPerSample"_s, value.toInt());
            break;
        case Qmmp::FORMAT_NAME:
            obj.insert(u"formatName"_s, value);
            break;
        case Qmmp::DECODER:
            obj.insert(u"decoder"_s, value);
            break;
        case Qmmp::FILE_SIZE:
            obj.insert(u"fileSize"_s, value.toLongLong());
            break;
        default:
            ;
        }

        ++it;
    }

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

bool Library::scanDirectories(const QStringList &paths)
{
    m_stopped = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, CONNECTION_NAME);
        db.setDatabaseName(Qmmp::configDir() + u"/library.sqlite"_s);
        db.open();

        readIgnoredFiles();

        QSqlQuery query(db);
        query.exec(u"PRAGMA journal_mode = WAL"_s);
        query.exec(u"PRAGMA synchronous = NORMAL"_s);

        for(const QString &path : std::as_const(paths))
        {
            addDirectory(path);
            if(m_stopped)
                break;
        }

        if(!m_stopped)
            removeMissingFiles(paths);

        db.close();
    }

    QSqlDatabase::removeDatabase(CONNECTION_NAME);
    qCDebug(plugin, "directory scan finished");
    return !m_stopped;
}

void Library::addDirectory(const QString &s)
{
    QList<QPair<TrackInfo, QString>> tracks;
    QStringList ignoredPaths;

    QDir dir(s);
    dir.setFilter(QDir::Files | QDir::Hidden);
    dir.setSorting(QDir::Name);
    QFileInfoList l = dir.entryInfoList(m_filters);

    for(const QFileInfo &info : std::as_const(l))
    {
        if(!checkFile(info) && !m_ignoredFiles.contains(info.filePath()))
        {
            QStringList paths;
            const QList<TrackInfo> pl = MetaDataManager::instance()->createPlayList(info.absoluteFilePath(), TrackInfo::AllParts, &paths);

            //save local file path
            for(const TrackInfo &t : std::as_const(pl))
                tracks << qMakePair(t, info.absoluteFilePath());
            ignoredPaths << paths;

        }

        if(m_stopped)
        {
            return;
        }
    }

    removeIgnoredTracks(&tracks, ignoredPaths);

    for(const auto &t : std::as_const(tracks))
        addTrack(t.first, t.second);


    updateIgnoredFiles(ignoredPaths);

    //filter directories
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name);
    l.clear();
    l = dir.entryInfoList();

    for(const QFileInfo &i : std::as_const(l))
    {
        addDirectory(i.absoluteFilePath());
        if(m_stopped)
            return;
    }
}

void Library::removeMissingFiles(const QStringList &paths)
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
        return;

    QSqlQuery query(db);
    if(!query.exec(u"SELECT FilePath,URL FROM track_library"_s))
    {
        qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
        return;
    }

    QString previousPath;

    while(query.next())
    {
        QString path = query.value(0).toString();
        QString url = query.value(1).toString();
        if(previousPath == path)
            continue;

        previousPath = path;

        if(!QFile::exists(path) || //remove missing or disabled file paths
                !std::any_of(paths.cbegin(), paths.cend(), [path](const QString &p){ return path.startsWith(p); } ) ||
                (!url.contains(u"://"_s) && m_ignoredFiles.contains(url))) //remove ignored files
        {
            qCDebug(plugin, "removing '%s' from library", qPrintable(path));
            QSqlQuery rmQuery(db);
            rmQuery.prepare(u"DELETE FROM track_library WHERE FilePath = :filepath"_s);
            rmQuery.bindValue(u":filepath"_s, path);
            if(!rmQuery.exec())
            {
                qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
                return;
            }
        }
    }

    if(!query.exec(u"SELECT FilePath FROM ignored_files"_s))
    {
        qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
        return;
    }

    while(query.next())
    {
        QString path = query.value(0).toString();

        if(!QFile::exists(path) || //remove missing or disabled file paths
                !std::any_of(paths.cbegin(), paths.cend(), [path](const QString &p){ return path.startsWith(p); } ))
        {
            qCDebug(plugin, "removing '%s' from ignored files", qPrintable(path));
            QSqlQuery rmQuery(db);
            rmQuery.prepare(u"DELETE FROM ignored_files WHERE FilePath = :filepath"_s);
            rmQuery.bindValue(u":filepath"_s, path);
            if(!rmQuery.exec())
            {
                qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
                return;
            }
        }
    }
}

bool Library::checkFile(const QFileInfo &info)
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
        return false;

    QSqlQuery query(db);
    query.prepare(u"SELECT Timestamp FROM track_library WHERE FilePath = :filepath"_s);
    query.bindValue(u":filepath"_s, info.absoluteFilePath());
    if(!query.exec())
    {
        qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
        return false;
    }
    if(!query.next())
        return false;

    return info.lastModified() == query.value(u"Timestamp"_s).toDateTime();
}

void Library::removeIgnoredTracks(QList<QPair<TrackInfo, QString>> *tracks, const QStringList &ignoredPaths)
{
    if(ignoredPaths.isEmpty())
        return;

    QSet<QString> ignoredPathsSet(ignoredPaths.cbegin(), ignoredPaths.cend());

    auto it = tracks->begin();
    while(it != tracks->end())
    {
        if(ignoredPathsSet.contains((*it).second))
        {
            it = tracks->erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Library::updateIgnoredFiles(const QStringList &paths)
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
        return;

    for(const QString &path : std::as_const(paths))
    {
        QSqlQuery query(db);
        query.prepare(u"INSERT OR REPLACE INTO ignored_files VALUES((SELECT ID FROM track_library WHERE FilePath = :filepath), :filepath)"_s);
        query.bindValue(u":filepath"_s, path);
        if(!query.exec())
        {
            qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
            break;
        }
    }
}

void Library::readIgnoredFiles()
{
    m_ignoredFiles.clear();

    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
        return;

    QSqlQuery query(db);
    if(!query.exec(u"SELECT FilePath FROM ignored_files"_s))
    {
        qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
        return;
    }

    while(query.next())
        m_ignoredFiles.insert(query.value(0).toString());
}
