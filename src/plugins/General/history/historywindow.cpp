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

#include <QStringList>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QSettings>
#include <QProgressBar>
#include <QTreeWidgetItem>
#include <QFile>
#include <QMenu>
#include <qmmp/qmmp.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/detailsdialog.h>
#include "historywindow.h"
#include "progressbaritemdelegate.h"
#include "ui_historywindow.h"

#define PathRole (Qt::UserRole + 4)
#define IdRole (Qt::UserRole + 5)

HistoryWindow::HistoryWindow(QSqlDatabase db, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::HistoryWindow)
{
    m_ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);
    m_db = db;

    QDateTime t = QDateTime::currentDateTime();
    t.setTime(QTime(23, 59, 0 ,0));
    m_ui->toDateEdit->setDateTime(t);
    t.setTime(QTime(0, 0, 0, 0));
    t = t.addDays(-7);
    m_ui->fromDateEdit->setDateTime(t);
    m_ui->distributionTreeWidget->setItemDelegate(new ProgressBarItemDelegate(this));
    m_ui->topArtistsTreeWidget->setItemDelegate(new ProgressBarItemDelegate(this));
    m_ui->topSongsTreeWidget->setItemDelegate(new ProgressBarItemDelegate(this));
    m_ui->topGenresTreeWidget->setItemDelegate(new ProgressBarItemDelegate(this));
    m_ui->historyTreeWidget->header()->setSortIndicator(0, Qt::AscendingOrder);
    m_ui->historyTreeWidget->header()->setSortIndicatorShown(true);
    m_ui->historyTreeWidget->header()->setSectionsClickable(true);
    m_ui->historyTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    readSettings();
    connect(m_ui->historyTreeWidget->header(), &QHeaderView::sortIndicatorChanged, this, &HistoryWindow::onSortIndicatorChanged);
    m_order = m_ui->historyTreeWidget->header()->sortIndicatorOrder();

    on_executeButton_clicked();
}

HistoryWindow::~HistoryWindow()
{
    delete m_ui;
}

void HistoryWindow::loadHistory()
{
    m_ui->historyTreeWidget->clear();

    if(!m_db.isOpen())
        return;

    QSqlQuery query(m_db);

    if(m_ui->historyTreeWidget->header()->sortIndicatorOrder() == Qt::DescendingOrder)
    {
        query.prepare(u"SELECT Timestamp,Title,Artist,AlbumArtist,Album,Comment,Genre,Composer,Track,Year,Duration,URL,ID "
                      "FROM track_history WHERE Timestamp BETWEEN :from and :to ORDER BY id DESC"_s);
    }
    else
    {
        query.prepare(u"SELECT Timestamp,Title,Artist,AlbumArtist,Album,Comment,Genre,Composer,Track,Year,Duration,URL,ID "
                      "FROM track_history WHERE Timestamp BETWEEN :from and :to"_s);
    }
    query.bindValue(u":from"_s, m_ui->fromDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));
    query.bindValue(u":to"_s, m_ui->toDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));

    if(!query.exec())
    {
        qCWarning(plugin, "query error: %s", qPrintable(query.lastError().text()));
        return;
    }

    QColor bgColor = palette().color(QPalette::Highlight);
    QColor textColor = palette().color(QPalette::HighlightedText);

    while (query.next())
    {
        TrackInfo info;
        info.setValue(Qmmp::TITLE,  query.value(1).toString());
        info.setValue(Qmmp::ARTIST,  query.value(2).toString());
        info.setValue(Qmmp::ALBUMARTIST,  query.value(3).toString());
        info.setValue(Qmmp::ALBUM,  query.value(4).toString());
        info.setValue(Qmmp::COMMENT,  query.value(5).toString());
        info.setValue(Qmmp::GENRE,  query.value(6).toString());
        info.setValue(Qmmp::COMPOSER,  query.value(7).toString());
        info.setValue(Qmmp::TRACK,  query.value(8).toString());
        info.setValue(Qmmp::YEAR,  query.value(9).toString());
        info.setDuration(query.value(10).toInt());
        info.setPath(query.value(11).toString());

        QDateTime dateTime = QDateTime::fromString(query.value(0).toString(), u"yyyy-MM-dd hh:mm:ss"_s);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        dateTime.setTimeZone(QTimeZone::UTC);
#else
        dateTime.setTimeSpec(Qt::UTC);
#endif
        QString dateStr = dateTime.toLocalTime().toString(tr("dd MMMM yyyy"));
        QString timeStr = dateTime.toLocalTime().toString(tr("hh:mm:ss"));
        int topLevelCount = m_ui->historyTreeWidget->topLevelItemCount();

        if(!topLevelCount || m_ui->historyTreeWidget->topLevelItem(topLevelCount - 1)->text(0) != dateStr)
        {
            m_ui->historyTreeWidget->addTopLevelItem(new QTreeWidgetItem());
            m_ui->historyTreeWidget->topLevelItem(topLevelCount++)->setText(0, dateStr);
            m_ui->historyTreeWidget->topLevelItem(topLevelCount - 1)->setFirstColumnSpanned(true);
            m_ui->historyTreeWidget->topLevelItem(topLevelCount - 1)->setTextAlignment(0, Qt::AlignCenter);
            m_ui->historyTreeWidget->topLevelItem(topLevelCount - 1)->setBackground(0, bgColor);
            m_ui->historyTreeWidget->topLevelItem(topLevelCount - 1)->setForeground(0, textColor);
        }

        QTreeWidgetItem *topLevelItem = m_ui->historyTreeWidget->topLevelItem(topLevelCount - 1);
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, timeStr);
        item->setText(1, m_formatter.format(info));
        item->setData(1, PathRole, info.path());
        item->setData(1, IdRole, query.value(12).toLongLong());
        topLevelItem->addChild(item);
    }

    m_ui->historyTreeWidget->expandAll();
}

void HistoryWindow::loadDistribution()
{
    m_ui->distributionTreeWidget->clear();

    if(!m_db.isOpen())
        return;

    QSqlQuery query(m_db);

    query.prepare(u"SELECT max(c) FROM( SELECT count(*) as c FROM track_history WHERE Timestamp BETWEEN :from and :to "
                  "GROUP BY date(Timestamp, 'localtime'))"_s);
    query.bindValue(u":from"_s, m_ui->fromDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));
    query.bindValue(u":to"_s, m_ui->toDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));
    if(!query.exec())
    {
        qCWarning(plugin, "query error: %s", qPrintable(query.lastError().text()));
        return;
    }
    if(!query.next())
    {
        qCWarning(plugin, "empty result");
        return;
    }
    int maxCount = query.value(0).toInt();
    query.finish();

    query.prepare(u"SELECT count(*), date(Timestamp, 'localtime') FROM track_history WHERE Timestamp BETWEEN :from and :to "
                  "GROUP BY date(Timestamp, 'localtime')"_s);
    query.bindValue(u":from"_s, m_ui->fromDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));
    query.bindValue(u":to"_s, m_ui->toDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));
    if(!query.exec())
    {
        qCWarning(plugin, "query error: %s", qPrintable(query.lastError().text()));
        return;
    }

    QColor bgColor = palette().color(QPalette::Highlight);
    QColor textColor = palette().color(QPalette::HighlightedText);

    while (query.next())
    {
        QDate date = QDate::fromString(query.value(1).toString(), u"yyyy-MM-dd"_s);
        QString monthStr = date.toString(tr("MM-yyyy"));
        QString dayStr = date.toString(tr("dd MMMM"));
        int topLevelCount = m_ui->distributionTreeWidget->topLevelItemCount();

        if(!topLevelCount || m_ui->distributionTreeWidget->topLevelItem(topLevelCount - 1)->text(0) != monthStr)
        {
            m_ui->distributionTreeWidget->addTopLevelItem(new QTreeWidgetItem());
            m_ui->distributionTreeWidget->topLevelItem(topLevelCount++)->setText(0,  monthStr);
            m_ui->distributionTreeWidget->topLevelItem(topLevelCount - 1)->setFirstColumnSpanned(true);
            m_ui->distributionTreeWidget->topLevelItem(topLevelCount - 1)->setTextAlignment(0, Qt::AlignCenter);
            m_ui->distributionTreeWidget->topLevelItem(topLevelCount - 1)->setBackground(0, bgColor);
            m_ui->distributionTreeWidget->topLevelItem(topLevelCount - 1)->setForeground(0, textColor);
        }

        QTreeWidgetItem *topLevelItem = m_ui->distributionTreeWidget->topLevelItem(topLevelCount - 1);
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, dayStr);
        topLevelItem->addChild(item);
        item->setData(1, ProgressBarRole, true);
        item->setData(1, ProgressBarMaxRole, maxCount);
        item->setData(1, ProgressBarValueRole, query.value(0).toInt());
        //item->setData(1, PathRole, info.path());
    }

    m_ui->distributionTreeWidget->expandAll();
}

void HistoryWindow::loadTopSongs()
{
    m_ui->topSongsTreeWidget->clear();

    if(!m_db.isOpen())
        return;

    QSqlQuery query(m_db);

    query.prepare(u"SELECT count(*) as c,Timestamp,Title,Artist,AlbumArtist,Album,Comment,Genre,Composer,Track,Year,Duration,URL "
                  "FROM track_history WHERE Timestamp BETWEEN :from and :to "
                  "GROUP BY Artist,Title ORDER BY c DESC LIMIT 100"_s);
    query.bindValue(u":from"_s, m_ui->fromDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));
    query.bindValue(u":to"_s, m_ui->toDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));

    if(!query.exec())
    {
        qCWarning(plugin, "query error: %s", qPrintable(query.lastError().text()));
        return;
    }

    int maxCount = 0;

    while(query.next())
    {
        TrackInfo info;
        info.setValue(Qmmp::TITLE,  query.value(2).toString());
        info.setValue(Qmmp::ARTIST,  query.value(3).toString());
        info.setValue(Qmmp::ALBUMARTIST,  query.value(4).toString());
        info.setValue(Qmmp::ALBUM,  query.value(5).toString());
        info.setValue(Qmmp::COMMENT,  query.value(6).toString());
        info.setValue(Qmmp::GENRE,  query.value(7).toString());
        info.setValue(Qmmp::COMPOSER,  query.value(8).toString());
        info.setValue(Qmmp::TRACK,  query.value(9).toString());
        info.setValue(Qmmp::YEAR,  query.value(10).toString());
        info.setDuration(query.value(11).toInt());
        info.setPath(query.value(12).toString());

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, m_formatter.format(info));

        m_ui->topSongsTreeWidget->addTopLevelItem(item);

        if(!maxCount)
            maxCount = query.value(0).toInt();

        item->setData(1, ProgressBarRole, true);
        item->setData(1, ProgressBarMaxRole, maxCount);
        item->setData(1, ProgressBarValueRole, query.value(0).toInt());
        item->setData(1, PathRole, info.path());
    }
}

void HistoryWindow::loadTopArtists()
{
    m_ui->topArtistsTreeWidget->clear();

    if(!m_db.isOpen())
        return;

    QSqlQuery query(m_db);

    query.prepare(u"SELECT count(*) as c,Artist "
                  "FROM track_history WHERE (Timestamp BETWEEN :from and :to) AND Artist NOT NULL "
                  "GROUP BY Artist ORDER BY c DESC LIMIT 100"_s);
    query.bindValue(u":from"_s, m_ui->fromDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));
    query.bindValue(u":to"_s, m_ui->toDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));

    if(!query.exec())
    {
        qCWarning(plugin, "query error: %s", qPrintable(query.lastError().text()));
        return;
    }

    int maxCount = 0;

    while (query.next())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, query.value(1).toString());

        m_ui->topArtistsTreeWidget->addTopLevelItem(item);

        if(!maxCount)
            maxCount = query.value(0).toInt();

        item->setData(1, ProgressBarRole, true);
        item->setData(1, ProgressBarMaxRole, maxCount);
        item->setData(1, ProgressBarValueRole, query.value(0).toInt());
    }
}

void HistoryWindow::loadTopGenres()
{
    m_ui->topGenresTreeWidget->clear();

    if(!m_db.isOpen())
        return;

    QSqlQuery query(m_db);

    query.prepare(u"SELECT count(*) as c,Genre "
                  "FROM track_history WHERE (Timestamp BETWEEN :from and :to) AND Genre NOT NULL "
                  "GROUP BY Genre ORDER BY c DESC LIMIT 100"_s);
    query.bindValue(u":from"_s, m_ui->fromDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));
    query.bindValue(u":to"_s, m_ui->toDateEdit->dateTime().toUTC().toString(u"yyyy-MM-dd hh:mm:ss"_s));

    if(!query.exec())
    {
        qCWarning(plugin, "query error: %s", qPrintable(query.lastError().text()));
        return;
    }

    int maxCount = 0;

    while (query.next())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, query.value(1).toString());

        m_ui->topGenresTreeWidget->addTopLevelItem(item);

        if(!maxCount)
            maxCount = query.value(0).toInt();

        item->setData(1, ProgressBarRole, true);
        item->setData(1, ProgressBarMaxRole, maxCount);
        item->setData(1, ProgressBarValueRole, query.value(0).toInt());
    }
}

void HistoryWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup(u"History"_s);
    restoreGeometry(settings.value(u"geometry"_s).toByteArray());
    m_ui->historyTreeWidget->header()->restoreState(settings.value(u"history_state"_s).toByteArray());
    m_ui->distributionTreeWidget->header()->restoreState(settings.value(u"distribution_state"_s).toByteArray());
    m_ui->topSongsTreeWidget->header()->restoreState(settings.value(u"top_songs_state"_s).toByteArray());
    m_ui->topArtistsTreeWidget->header()->restoreState(settings.value(u"top_artists_state"_s).toByteArray());
    m_ui->topGenresTreeWidget->header()->restoreState(settings.value(u"top_genres_state"_s).toByteArray());
    m_formatter.setPattern(settings.value(u"title_format"_s, u"%if(%p,%p - %t,%t)"_s).toString());
    settings.endGroup();
}

void HistoryWindow::removeTrack(QTreeWidgetItem *item)
{
    if(!m_db.isOpen())
        return;

    qint64 id = item->data(1, IdRole).toLongLong();

    QSqlQuery query(m_db);

    query.prepare(u"DELETE FROM track_history WHERE ID=:id"_s);
    query.bindValue(u":id"_s, id);
    if(query.exec())
        delete item;
    else
        qCWarning(plugin, "query error: %s", qPrintable(query.lastError().text()));
}

void HistoryWindow::showInformation(QTreeWidgetItem *item)
{
    if(!m_db.isOpen())
        return;

    qint64 id = item->data(1, IdRole).toLongLong();

    QSqlQuery query(m_db);

    query.prepare(u"SELECT Timestamp,Title,Artist,AlbumArtist,Album,Comment,Genre,Composer,Track,Year,Duration,URL,ID "
                  "FROM track_history WHERE ID=:id"_s);
    query.bindValue(u":id"_s, id);

    if(!query.exec())
    {
        qCWarning(plugin, "query error: %s", qPrintable(query.lastError().text()));
        return;
    }

    if(query.next())
    {
        PlayListTrack track;
        track.setValue(Qmmp::TITLE, query.value(1).toString());
        track.setValue(Qmmp::ARTIST, query.value(2).toString());
        track.setValue(Qmmp::ALBUMARTIST, query.value(3).toString());
        track.setValue(Qmmp::ALBUM, query.value(4).toString());
        track.setValue(Qmmp::COMMENT, query.value(5).toString());
        track.setValue(Qmmp::GENRE, query.value(6).toString());
        track.setValue(Qmmp::COMPOSER, query.value(7).toString());
        track.setValue(Qmmp::TRACK, query.value(8).toString());
        track.setValue(Qmmp::YEAR, query.value(9).toString());
        track.setDuration(query.value(10).toInt());
        track.setPath(query.value(11).toString());

        DetailsDialog d(&track, this);
        d.exec();
    }
}

void HistoryWindow::closeEvent(QCloseEvent *)
{
    QSettings settings;
    settings.beginGroup(u"History"_s);
    settings.setValue(u"geometry"_s, saveGeometry());
    settings.setValue(u"history_state"_s, m_ui->historyTreeWidget->header()->saveState());
    settings.setValue(u"distribution_state"_s, m_ui->distributionTreeWidget->header()->saveState());
    settings.setValue(u"top_songs_state"_s, m_ui->topSongsTreeWidget->header()->saveState());
    settings.setValue(u"top_artists_state"_s, m_ui->topArtistsTreeWidget->header()->saveState());
    settings.setValue(u"top_genres_state"_s, m_ui->topGenresTreeWidget->header()->saveState());
    settings.endGroup();
}

void HistoryWindow::on_executeButton_clicked()
{
    loadHistory();
    loadDistribution();
    loadTopSongs();
    loadTopArtists();
    loadTopGenres();
}

void HistoryWindow::on_lastWeekButton_clicked()
{
    QDateTime t = QDateTime::currentDateTime();
    t.setTime(QTime(23, 59, 0 ,0));
    m_ui->toDateEdit->setDateTime(t);
    t.setTime(QTime(0, 0, 0, 0));
    t = t.addDays(-t.date().dayOfWeek() + 1);
    m_ui->fromDateEdit->setDateTime(t);
    on_executeButton_clicked();
}

void HistoryWindow::on_lastMonthButton_clicked()
{
    QDateTime t = QDateTime::currentDateTime();
    t.setTime(QTime(23, 59, 0 ,0));
    m_ui->toDateEdit->setDateTime(t);
    t.setTime(QTime(0, 0, 0, 0));
    t.setDate(QDate(t.date().year(), t.date().month(), 1)); ;
    m_ui->fromDateEdit->setDateTime(t);
    on_executeButton_clicked();
}

void HistoryWindow::on_historyTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int)
{
    if(item && item->parent())
        on_topSongsTreeWidget_itemDoubleClicked(item, 0);
}

void HistoryWindow::on_historyTreeWidget_customContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem *item = m_ui->historyTreeWidget->itemAt(pos);
    if(item && item->parent())
    {
        QString path = item->data(1, PathRole).toString();
        QMenu menu(this);
        menu.addAction(QIcon::fromTheme(u"list-add"_s),tr("Add to Playlist"), this, [=] { PlayListManager::instance()->addPath(path); } );
        menu.addAction(QIcon::fromTheme(u"dialog-information"_s), tr("&View Track Details"), this, [=] { showInformation(item); });
        menu.addSeparator();
        menu.addAction(QIcon::fromTheme(u"edit-delete"_s), tr("Remove from History"), this, [=] { removeTrack(item); } );
        menu.exec(m_ui->historyTreeWidget->viewport()->mapToGlobal(pos));
    }
}

void HistoryWindow::on_topSongsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int)
{
    QString path = item->data(1, PathRole).toString();
    if(!path.contains(u"://"_s) && !QFile::exists(path))
    {
        qCDebug(plugin, "unable to find file: %s", qPrintable(path));
        return;
    }

    PlayListManager *plManager = PlayListManager::instance();
    plManager->clear();
    if(!plManager->selectedPlayList()->isLoaderRunning())
    {
        plManager->activatePlayList(plManager->selectedPlayList());
        connect(plManager->currentPlayList(), &PlayListModel::tracksAdded, this, &HistoryWindow::playTrack);
        connect(plManager->currentPlayList(), &PlayListModel::loaderFinished, this, &HistoryWindow::disconnectPl);

    }
    plManager->addPath(path);
}

void HistoryWindow::onSortIndicatorChanged(int index, Qt::SortOrder order)
{
    if(index == 0)
    {
        m_order = order;
        loadHistory();
    }
    else //ignore sorting for second section
    {
        //restore sort indicator order
        m_ui->historyTreeWidget->header()->setSortIndicator(0, m_order);
    }
}

void HistoryWindow::playTrack(const QList<PlayListTrack *> &tracks)
{
    PlayListManager *plManager = PlayListManager::instance();
    PlayListModel *model = qobject_cast<PlayListModel*>(sender());
    plManager->selectPlayList(model);
    plManager->activatePlayList(model);
    disconnect(model, &PlayListModel::tracksAdded, this, &HistoryWindow::playTrack);
    if(plManager->currentPlayList()->setCurrent(tracks.constFirst()))
    {
        MediaPlayer::instance()->stop();
        MediaPlayer::instance()->play();
    }
}

void HistoryWindow::disconnectPl()
{
    PlayListModel *model = qobject_cast<PlayListModel*>(sender());
    disconnect(model, &PlayListModel::tracksAdded, this, &HistoryWindow::playTrack);
}
