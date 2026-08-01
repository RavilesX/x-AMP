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

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>
#include <QMimeData>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWidget>
#include <QMessageBox>
#include <qmmp/qmmp.h>
#include <qmmp/soundcore.h>
#include <qmmpui/playlistparser.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/detailsdialog.h>
#include <qmmpui/mediaplayer.h>
#include "librarymodel.h"

#define CONNECTION_NAME u"qmmp_library_view"_s

class LibraryTreeItem
{
public:
    LibraryTreeItem() {}
    ~LibraryTreeItem()
    {
        clear();
    }

    void clear()
    {
        name.clear();
        type = Qmmp::UNKNOWN;
        parent = nullptr;
        qDeleteAll(children);
        children.clear();
    }

    QString name;
    int year = 0;
    Qmmp::MetaData type = Qmmp::UNKNOWN;
    QList<LibraryTreeItem *> children;
    LibraryTreeItem *parent = nullptr;
};

LibraryModel::LibraryModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_rootItem = new LibraryTreeItem;
    QSettings settings;
    m_showYear = settings.value(u"Library/show_year"_s, false).toBool();
    refresh();
}

LibraryModel::~LibraryModel()
{
    delete m_rootItem;

    if(QSqlDatabase::contains(CONNECTION_NAME))
    {
        QSqlDatabase::database(CONNECTION_NAME).close();
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
}

Qt::ItemFlags LibraryModel::flags(const QModelIndex &index) const
{
    if(index.isValid())
        return QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled;

    return QAbstractItemModel::flags(index);
}

QStringList LibraryModel::mimeTypes() const
{
    return { u"application/json"_s };
}

QMimeData *LibraryModel::mimeData(const QModelIndexList &indexes) const
{
    QList<PlayListTrack *> tracks = getTracks(indexes);

    if(!tracks.isEmpty())
    {
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(u"application/json"_s, PlayListParser::serialize(tracks));
        qDeleteAll(tracks);
        return mimeData;
    }

    return nullptr;
}

bool LibraryModel::canFetchMore(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return false;

    LibraryTreeItem *parentItem = static_cast<LibraryTreeItem *>(parent.internalPointer());
    if(parentItem == m_rootItem || parentItem->type == Qmmp::TITLE)
        return false;

    return parentItem->children.isEmpty();
}

void LibraryModel::fetchMore(const QModelIndex &parent)
{
    if(!parent.isValid())
        return;

    LibraryTreeItem *parentItem = static_cast<LibraryTreeItem *>(parent.internalPointer());

    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
        return;

    if(parentItem->type == Qmmp::ARTIST)
    {
        QSqlQuery query(db);
        if(m_filter.isEmpty())
        {
            query.prepare(u"SELECT DISTINCT Album, Year from track_library WHERE Artist = :artist"_s);
        }
        else
        {
            query.prepare(u"SELECT DISTINCT Album, Year from track_library WHERE Artist = :artist AND SearchString LIKE :filter"_s);
            query.bindValue(u":filter"_s, QStringLiteral("%%1%").arg(m_filter.toLower()));
        }
        query.bindValue(u":artist"_s, parentItem->name);

        if(!query.exec())
        {
            qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
            return;
        }

        while(query.next())
        {
            LibraryTreeItem *item = new LibraryTreeItem;
            item->name = query.value(u"Album"_s).toString();
            item->year = query.value(u"Year"_s).toInt();
            item->type = Qmmp::ALBUM;
            item->parent = parentItem;
            parentItem->children << item;
        }
    }
    else if(parentItem->type == Qmmp::ALBUM)
    {
        QSqlQuery query(db);
        if(m_filter.isEmpty())
        {
            query.prepare(u"SELECT Title from track_library WHERE Artist = :artist AND Album = :album"_s);
        }
        else
        {
            query.prepare(u"SELECT Title from track_library WHERE Artist = :artist AND Album = :album "
                          "AND SearchString LIKE :filter"_s);
            query.bindValue(u":filter"_s, QStringLiteral("%%1%").arg(m_filter.toLower()));
        }
        query.bindValue(u":artist"_s, parentItem->parent->name);
        query.bindValue(u":album"_s, parentItem->name);

        if(!query.exec())
        {
            qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
            return;
        }

        while(query.next())
        {
            LibraryTreeItem *item = new LibraryTreeItem;
            item->name = query.value(u"Title"_s).toString();
            item->type = Qmmp::TITLE;
            item->parent = parentItem;
            parentItem->children << item;
        }
    }
}

QVariant LibraryModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    LibraryTreeItem *item = static_cast<LibraryTreeItem *>(index.internalPointer());
    if(item->type == Qmmp::ALBUM && m_showYear && item->year > 0)
        return tr("%1 - %2").arg(item->year).arg(item->name);

    return item->name;
}

QModelIndex LibraryModel::parent(const QModelIndex &child) const
{
    if(!child.isValid())
        return QModelIndex();

    LibraryTreeItem *childItem = static_cast<LibraryTreeItem *>(child.internalPointer());
    LibraryTreeItem *parentItem = childItem->parent;

    if(parentItem == m_rootItem || !parentItem || !parentItem->parent)
        return QModelIndex();

    return createIndex(parentItem->parent->children.indexOf(parentItem), 0, parentItem);
}

QModelIndex LibraryModel::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid() && parent.column() != 0)
        return QModelIndex();

    LibraryTreeItem *parentItem = parent.isValid() ? static_cast<LibraryTreeItem *>(parent.internalPointer()) :
                                                     m_rootItem;

    if(row >= 0 && row < parentItem->children.count())
        return createIndex(row, column, parentItem->children.at(row));

    return QModelIndex();
}

int LibraryModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

int LibraryModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return m_rootItem->children.count();

    LibraryTreeItem *parentItem = static_cast<LibraryTreeItem *>(parent.internalPointer());
    if(parentItem->type == Qmmp::TITLE)
        return 0;

    return qMax(1, parentItem->children.count());
}

void LibraryModel::setFilter(const QString &filter)
{
    m_filter = filter;
}

void LibraryModel::refresh()
{
    beginResetModel();
    m_rootItem->clear();

    QSqlDatabase db;

    if(QSqlDatabase::contains(CONNECTION_NAME))
    {
        db = QSqlDatabase::database(CONNECTION_NAME);
    }
    else
    {
        db = QSqlDatabase::addDatabase(u"QSQLITE"_s, CONNECTION_NAME);
        db.setDatabaseName(Qmmp::configDir() + u"/library.sqlite"_s);
        db.open();
    }

    if(!db.isOpen())
    {
        endResetModel();
        return;
    }

    QSqlQuery query(db);
    if(m_filter.isEmpty())
    {
        query.prepare(u"SELECT DISTINCT Artist from track_library ORDER BY Artist"_s);
    }
    else
    {
        query.prepare(u"SELECT DISTINCT Artist from track_library WHERE SearchString LIKE :filter ORDER BY Artist"_s);
        query.bindValue(u":filter"_s, QStringLiteral("%%1%").arg(m_filter.toLower()));
    }

    if(!query.exec())
        qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));

    while(query.next())
    {
        LibraryTreeItem *item = new LibraryTreeItem;
        item->name = query.value(u"Artist"_s).toString();
        item->type = Qmmp::ARTIST;
        item->parent = m_rootItem;
        m_rootItem->children << item;
    }
    endResetModel();
}

void LibraryModel::add(const QModelIndexList &indexes)
{
    PlayListManager::instance()->addTracks(getTracks(indexes));
}

void LibraryModel::replace(const QModelIndexList &indexes)
{
    QList<PlayListTrack *> tracks = getTracks(indexes);
    if(tracks.isEmpty())
        return;

    SoundCore *core = SoundCore::instance();
    PlayListManager *manager = PlayListManager::instance();
    PlayListModel *model = PlayListManager::instance()->selectedPlayList();

    bool play = (core->state() == Qmmp::Playing || core->state() == Qmmp::Paused || core->state() == Qmmp::Buffering) &&
            model == manager->currentPlayList();

    model->clear();
    model->addTracks(tracks);

    if(play)
    {
        MediaPlayer::instance()->stop();
        MediaPlayer::instance()->play();
    }
}

void LibraryModel::showTrackInformation(const QModelIndexList &indexes, QWidget *parent)
{
    QList<PlayListTrack *> tracks = getTracks(indexes);
    if(tracks.isEmpty())
        return;

    DetailsDialog *dialog = new DetailsDialog(tracks, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->show();
    connect(dialog, &QObject::destroyed, [=]() { qDeleteAll(tracks); });
}

void LibraryModel::showLibraryInformation(QWidget *parent)
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    if(!db.isOpen())
    {
        QMessageBox::critical(parent, tr("Error"), tr("Unable to connect to database"));
        return;
    }

    QSqlQuery query(db);
    query.prepare(u"select COUNT(id),COUNT(DISTINCT Album||Artist),COUNT(DISTINCT Artist),SUM(Duration) from track_library"_s);

    if(!query.exec())
    {
        qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
        return;
    }
    query.next();

    int tracks = query.value(0).toInt();
    int albums = query.value(1).toInt();
    int artists = query.value(2).toInt();
    qint64 duration = query.value(3).toLongLong() / 1000;
    int days = duration / (3600 * 24);
    int hours = (duration / 3600) % 24;

    QString daysText = tr("%n day(s)", "", days);
    QString hoursText = tr("%n hour(s)", "", hours);
    QString minutesText = tr("%n minute(s)", "", duration % 3600 / 60);
    QString secondsText = tr("%n second(s)", "", duration % 60);
    QString durationText;

    if(days > 0)
        durationText = tr("%1 %2 %3 %4", "days hours minutes seconds").arg(daysText, hoursText, minutesText, secondsText);
    else if(hours > 0)
        durationText = tr("%1 %2 %3", "hours minutes seconds").arg(hoursText, minutesText, secondsText);
    else
        durationText = tr("%1 %2", "minutes seconds").arg(minutesText, secondsText);

    QStringList lines = {
        tr("Number of tracks: <b>%1</b>").arg(tracks),
        tr("Number of albums: <b>%1</b>").arg(albums),
        tr("Number of artists: <b>%1</b>").arg(artists),
        tr("Total duration: <b>%1</b>").arg(durationText),
    };

    QMessageBox::information(parent, tr("Library Information"), lines.join(u"<br>"_s));
}

QList<PlayListTrack *> LibraryModel::getTracks(const QModelIndexList &indexes) const
{
    QList<PlayListTrack *> tracks;

    for(const QModelIndex &index : indexes)
    {
        if(index.isValid())
        {
            tracks << getTracks(index);
        }
    }

    return tracks;
}

QList<PlayListTrack *> LibraryModel::getTracks(const QModelIndex &index) const
{
    QSqlDatabase db = QSqlDatabase::database(CONNECTION_NAME);
    QList<PlayListTrack *> tracks;
    if(!db.isOpen())
        return tracks;

    const LibraryTreeItem *item = static_cast<const LibraryTreeItem *>(index.internalPointer());

    if(item->type == Qmmp::TITLE)
    {
        QSqlQuery query(db);
        query.prepare(u"SELECT * from track_library WHERE Artist = :artist AND Album = :album AND Title = :title"_s);
        query.bindValue(u":artist"_s, item->parent->parent->name);
        query.bindValue(u":album"_s, item->parent->name);
        query.bindValue(u":title"_s, item->name);

        if(!query.exec())
        {
            qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
            return tracks;
        }

        if(query.next())
        {
            tracks << createTrack(query);
        }
    }
    else if(item->type == Qmmp::ALBUM)
    {
        QSqlQuery query(db);
        query.prepare(u"SELECT * from track_library WHERE Artist = :artist AND Album = :album"_s);
        query.bindValue(u":artist"_s, item->parent->name);
        query.bindValue(u":album"_s, item->name);

        if(!query.exec())
        {
            qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
            return tracks;
        }

        while(query.next())
        {
           tracks << createTrack(query);
        }
    }
    else if(item->type == Qmmp::ARTIST)
    {
        QSqlQuery query(db);
        query.prepare(u"SELECT * from track_library WHERE Artist = :artist"_s);
        query.bindValue(u":artist"_s, item->name);

        if(!query.exec())
        {
            qCWarning(plugin, "exec error: %s", qPrintable(query.lastError().text()));
            return tracks;
        }

        while(query.next())
        {
            tracks << createTrack(query);
        }
    }

    return tracks;
}

PlayListTrack *LibraryModel::createTrack(const QSqlQuery &query) const
{
    static const QHash<Qmmp::MetaData , QString> metaColumns = {
        { Qmmp::TITLE, u"Title"_s },
        { Qmmp::ARTIST, u"Artist"_s },
        { Qmmp::ALBUMARTIST, u"AlbumArtist"_s },
        { Qmmp::ALBUM, u"Album"_s },
        { Qmmp::COMMENT, u"Comment"_s },
        { Qmmp::GENRE, u"Genre"_s },
        { Qmmp::COMPOSER, u"Composer"_s },
        { Qmmp::YEAR, u"Year"_s },
        { Qmmp::TRACK, u"Track"_s },
        { Qmmp::DISCNUMBER, u"DiscNumber"_s }
    };

    PlayListTrack *track = new PlayListTrack;
    track->setPath(query.value(u"URL"_s).toString());
    track->setDuration(query.value(u"Duration"_s).toLongLong());

    for(auto it = metaColumns.cbegin(); it != metaColumns.cend(); ++it)
    {
       QString value = query.value(it.value()).toString();
       track->setValue(it.key(), value);
    }

    QJsonDocument document = QJsonDocument::fromJson(query.value(u"AudioInfo"_s).toByteArray());
    QJsonObject obj = document.object();
    track->setValue(Qmmp::BITRATE, obj.value(u"bitrate"_s).toInt());
    track->setValue(Qmmp::SAMPLERATE, obj.value(u"samplerate"_s).toInt());
    track->setValue(Qmmp::CHANNELS, obj.value(u"channels"_s).toInt());
    track->setValue(Qmmp::BITS_PER_SAMPLE, obj.value(u"bitsPerSample"_s).toInt());
    track->setValue(Qmmp::FORMAT_NAME, obj.value(u"formatName"_s).toString());
    track->setValue(Qmmp::DECODER, obj.value(u"decoder"_s).toString());
    track->setValue(Qmmp::FILE_SIZE, qint64(obj.value(u"fileSize"_s).toDouble()));
    return track;
}
