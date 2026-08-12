/***************************************************************************
 *   Copyright (C) 2026 by x-AMP contributors                              *
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

#ifndef XUIQUEUEDIALOG_H
#define XUIQUEUEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <functional>

class QLabel;
class QPushButton;
class PlayListManager;
class PlayListModel;
class PlayListTrack;

/*!
 * The dialog's table, with rows that can be dragged into another order.
 *
 * Only the drop is taken over: Qt's own internal move works on the cells it
 * was handed rather than on whole rows, and the new order has to be written
 * back to the playlists' queues in any case. Reports where the drop landed
 * and leaves the moving to \b onDrop.
 */
class QueueTable : public QTableWidget
{
    Q_OBJECT
public:
    QueueTable(int columns, QWidget *parent = nullptr);

    /*! Called with the row the selection was dropped before. */
    std::function<void(int row)> onDrop;

protected:
    void dropEvent(QDropEvent *) override;
};

/*!
 * The play queue, listed and edited in one place.
 *
 * One queue serves every playlist, and a queued track is otherwise only
 * visible as a number beside its row. This lists the whole of it, in the
 * order it will be played, naming the playlist each track comes from.
 *
 * Nothing here is painted by hand: it is a plain dialog, so it picks up the
 * surfaces from the interface's style sheet and loses its frame to
 * XUiDialogs, exactly as the preferences do.
 */
class XUiQueueDialog : public QDialog
{
    Q_OBJECT
public:
    explicit XUiQueueDialog(PlayListManager *manager, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    /*! Reads the queues again and refills the table. */
    void reload();
    /*! Drops the selected rows from their queues. Bound to Delete. */
    void removeSelected();
    /*! Rows of the table holding a selected cell, in order. */
    QList<int> selectedRows() const;
    /*! Puts the selected rows before \b row and writes the queues again. */
    void moveSelectedTo(int row);
    void clearQueue();
    /*! Copies the queue, in order, into a playlist of its own. */
    void saveAsPlayList();
    /*! Lists the playlists the queue can be added to, as the menu opens. */
    void fillPlayListMenu();
    /*! The whole queue, in order, as tracks a playlist can take ownership of. */
    QList<PlayListTrack *> copyOfQueue() const;
    /*! Keeps the total sitting under the duration column. */
    void alignTotal();
    /*! Removes \b tracks from their queues with a single view update. */
    static void dequeue(PlayListModel *model, const QList<PlayListTrack *> &tracks);

    /*! One queued track, kept with the playlist that owns it. */
    struct Entry
    {
        PlayListModel *model = nullptr;
        PlayListTrack *track = nullptr;
    };

    PlayListManager *m_manager;
    QueueTable *m_table;
    QLabel *m_total;
    QWidget *m_totalSpacer; //stands in for the table's scrollbar
    QPushButton *m_clear;
    QPushButton *m_save;
    QPushButton *m_add;
    QList<Entry> m_entries;
};

#endif
