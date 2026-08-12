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

#include <QApplication>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QVBoxLayout>
#include <qmmpui/metadataformatter.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlisttrack.h>
#include "xuitheme.h"
#include "xuiqueuedialog.h"

namespace
{
    enum Column { NumberColumn, FileColumn, PlayListColumn, DurationColumn, ColumnCount };
    constexpr int LOGO_SIZE = 26;
}

QueueTable::QueueTable(int columns, QWidget *parent)
    : QTableWidget(0, columns, parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(false); //a drop lands between rows, not on one
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);
}

void QueueTable::dropEvent(QDropEvent *e)
{
    //The base class moves the cells it was handed, which for a table means
    //one cell of the row rather than the row itself. The drop is only read
    //for its position here and the reordering is left to the dialog, which
    //has to rewrite the queues anyway.
    const QModelIndex index = indexAt(e->position().toPoint());
    int row = index.isValid() ? index.row() : rowCount();
    if(index.isValid() && dropIndicatorPosition() == QAbstractItemView::BelowItem)
        ++row;

    //IgnoreAction rather than the move being offered: after a move it thinks
    //it did not carry out itself, the view drops the dragged rows on its own,
    //and by then the table has already been filled again
    e->setDropAction(Qt::IgnoreAction);
    e->accept();

    //queued, so the rows are rebuilt once the drag has finished unwinding
    //rather than underneath it
    if(onDrop)
    {
        const std::function<void(int)> handler = onDrop;
        QMetaObject::invokeMethod(this, [handler, row] { handler(row); },
                                  Qt::QueuedConnection);
    }
}

XUiQueueDialog::XUiQueueDialog(PlayListManager *manager, QWidget *parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle(tr("Queue"));
    resize(760, 440);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(XUi::CardPadding, XUi::CardPadding,
                               XUi::CardPadding, XUi::CardPadding);
    layout->setSpacing(XUi::CardGap);

    //header: the dialog has no title bar of its own to name it, so the name
    //goes here, with the application's logo closing the row on the right
    QHBoxLayout *header = new QHBoxLayout;
    QLabel *title = new QLabel(tr("Queue"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.15);
    title->setFont(titleFont);
    header->addWidget(title);
    header->addStretch(1);

    const QIcon logo = qApp->windowIcon();
    if(!logo.isNull())
    {
        QLabel *mark = new QLabel(this);
        mark->setPixmap(logo.pixmap(QSize(LOGO_SIZE, LOGO_SIZE), devicePixelRatioF()));
        header->addWidget(mark, 0, Qt::AlignRight | Qt::AlignVCenter);
    }
    layout->addLayout(header);

    m_table = new QueueTable(ColumnCount, this);
    m_table->onDrop = [this](int row) { moveSelectedTo(row); };
    m_table->setHorizontalHeaderLabels({ tr("#"), tr("File"), tr("Playlist"), tr("Duration") });
    m_table->horizontalHeaderItem(DurationColumn)
           ->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setWordWrap(false);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->horizontalHeader()->setSectionResizeMode(NumberColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(FileColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(PlayListColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(DurationColumn, QHeaderView::ResizeToContents);
    //The interface fills a selected row with the accent, and against a pale
    //one the white text on top of it vanished. The row is marked by colouring
    //its own text with the accent instead, the way the playing track already
    //is, so it stays legible whatever colour the user picked.
    m_table->setStyleSheet(QStringLiteral("QTableView::item:selected {"
                                          "  background: transparent; color: %1;"
                                          "}").arg(XUi::Accent.name()));
    layout->addWidget(m_table, 1);

    //the total belongs under the column it sums up, so it is given that
    //column's width and pushed clear of the table's scrollbar
    QHBoxLayout *totalRow = new QHBoxLayout;
    totalRow->setContentsMargins(0, 0, 0, 0);
    totalRow->addStretch(1);
    m_total = new QLabel(this);
    m_total->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_total->setToolTip(tr("Total time"));
    totalRow->addWidget(m_total);
    m_totalSpacer = new QWidget(this);
    totalRow->addWidget(m_totalSpacer);
    layout->addLayout(totalRow);

    QHBoxLayout *buttons = new QHBoxLayout;
    m_clear = new QPushButton(tr("Clear Queue"), this);
    m_save = new QPushButton(tr("Save as Playlist..."), this);
    m_add = new QPushButton(tr("Add to Playlist"), this);
    //filled when it drops down, since a playlist can be added or renamed
    //while this dialog is open
    QMenu *targets = new QMenu(m_add);
    connect(targets, &QMenu::aboutToShow, this, &XUiQueueDialog::fillPlayListMenu);
    m_add->setMenu(targets);
    QPushButton *close = new QPushButton(tr("Close"), this);
    close->setDefault(true);
    buttons->addWidget(m_clear);
    buttons->addWidget(m_save);
    buttons->addWidget(m_add);
    buttons->addStretch(1);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    connect(m_clear, &QPushButton::clicked, this, &XUiQueueDialog::clearQueue);
    connect(m_save, &QPushButton::clicked, this, &XUiQueueDialog::saveAsPlayList);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    //the dialog is not modal to the engine: a track finishing pulls its own
    //entry out of the queue, and the table has to follow
    connect(m_manager, &PlayListManager::playListsChanged, this, &XUiQueueDialog::reload);

    QShortcut *del = new QShortcut(QKeySequence::Delete, m_table);
    del->setContext(Qt::WidgetShortcut);
    connect(del, &QShortcut::activated, this, &XUiQueueDialog::removeSelected);

    reload();
}

void XUiQueueDialog::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);
    alignTotal();
}

void XUiQueueDialog::reload()
{
    m_entries.clear();
    const QList<PlayListModel *> playLists = m_manager->playLists();
    for(PlayListModel *model : playLists)
    {
        //a queue emptied by playback is reported here as well
        connect(model, &PlayListModel::listChanged, this, &XUiQueueDialog::reload,
                Qt::UniqueConnection);
    }
    //one queue for every playlist, already in playing order
    const QList<PlayListTrack *> queued = m_manager->queuedTracks();
    for(PlayListTrack *track : queued)
        m_entries.append({ m_manager->queuedPlayList(track), track });

    m_table->clearContents();
    m_table->setRowCount(m_entries.count());
    qint64 total = 0;
    for(int row = 0; row < m_entries.count(); ++row)
    {
        const Entry &entry = m_entries.at(row);
        total += qMax<qint64>(0, entry.track->duration());

        QTableWidgetItem *number = new QTableWidgetItem(QString::number(row + 1));
        number->setForeground(XUi::TextFaint);
        m_table->setItem(row, NumberColumn, number);
        //the same text the playlist shows, so a row is recognised at a glance
        m_table->setItem(row, FileColumn,
                         new QTableWidgetItem(entry.track->formattedTitle(0)));
        QTableWidgetItem *list = new QTableWidgetItem(entry.model->name());
        list->setForeground(XUi::TextDim);
        m_table->setItem(row, PlayListColumn, list);
        QTableWidgetItem *duration = new QTableWidgetItem(entry.track->formattedDuration());
        duration->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, DurationColumn, duration);
    }

    m_total->setText(total > 0 ? MetaDataFormatter::formatDuration(total) : QString());
    m_clear->setEnabled(!m_entries.isEmpty());
    m_save->setEnabled(!m_entries.isEmpty());
    m_add->setEnabled(!m_entries.isEmpty());
    alignTotal();
}

void XUiQueueDialog::fillPlayListMenu()
{
    QMenu *menu = m_add->menu();
    menu->clear();
    const QList<PlayListModel *> playLists = m_manager->playLists();
    for(PlayListModel *model : playLists)
    {
        connect(menu->addAction(model->name()), &QAction::triggered, this, [this, model] {
            //the menu is built as it opens, but a playlist could still be
            //dropped by another window before the entry is picked
            if(m_manager->playLists().contains(model))
                model->addTracks(copyOfQueue());
        });
    }
}

QList<PlayListTrack *> XUiQueueDialog::copyOfQueue() const
{
    //copies, not the queued tracks themselves: a playlist owns what it holds,
    //and these are still owned by the playlists they were queued from
    QList<PlayListTrack *> copies;
    copies.reserve(m_entries.count());
    for(const Entry &entry : std::as_const(m_entries))
        copies << new PlayListTrack(entry.track);
    return copies;
}

void XUiQueueDialog::alignTotal()
{
    m_total->setFixedWidth(m_table->columnWidth(DurationColumn));
    QScrollBar *bar = m_table->verticalScrollBar();
    m_totalSpacer->setFixedWidth(bar && bar->isVisible() ? bar->width() : 0);
}

QList<int> XUiQueueDialog::selectedRows() const
{
    QList<int> rows;
    const QList<QTableWidgetItem *> items = m_table->selectedItems();
    for(QTableWidgetItem *item : items)
    {
        if(!rows.contains(item->row()))
            rows << item->row();
    }
    std::sort(rows.begin(), rows.end());
    return rows;
}

void XUiQueueDialog::moveSelectedTo(int row)
{
    const QList<int> rows = selectedRows();
    if(rows.isEmpty())
        return;

    QList<Entry> moved, kept;
    for(int i = 0; i < m_entries.count(); ++i)
        (rows.contains(i) ? moved : kept) << m_entries.at(i);

    //the drop position was read off the old list, so it has to come back by
    //however many of the rows being moved sat above it
    int at = row;
    for(int i : rows)
    {
        if(i < row)
            --at;
    }
    for(int i = 0; i < moved.count(); ++i)
        kept.insert(qBound(0, at + i, kept.count()), moved.at(i));

    QList<PlayListTrack *> order;
    order.reserve(kept.count());
    for(const Entry &entry : std::as_const(kept))
        order << entry.track;
    m_manager->reorderQueue(order);

    reload();
    //the dragged rows stay marked where they landed, so a second drag needs
    //no fresh selection
    QItemSelection selection;
    for(int i = 0; i < moved.count(); ++i)
    {
        const int landed = qBound(0, at + i, m_table->rowCount() - 1);
        selection.select(m_table->model()->index(landed, 0),
                         m_table->model()->index(landed, ColumnCount - 1));
    }
    m_table->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void XUiQueueDialog::removeSelected()
{
    const QList<int> rows = selectedRows();
    if(rows.isEmpty())
        return;

    //dequeueing goes through the playlist a track belongs to, so the rows are
    //grouped by playlist and each one is told once
    QHash<PlayListModel *, QList<PlayListTrack *>> byPlayList;
    for(int row : rows)
    {
        const Entry &entry = m_entries.at(row);
        if(entry.model)
            byPlayList[entry.model] << entry.track;
    }
    for(auto it = byPlayList.cbegin(); it != byPlayList.cend(); ++it)
        dequeue(it.key(), it.value());

    reload();
    //keep the keyboard on the row that took the place of the last one removed
    const int row = qMin(rows.first(), m_table->rowCount() - 1);
    if(row >= 0)
        m_table->selectRow(row);
}

void XUiQueueDialog::dequeue(PlayListModel *model, const QList<PlayListTrack *> &tracks)
{
    if(tracks.isEmpty())
        return;

    //PlayListModel reports every single track, and each report repaints every
    //view; only the last step of the batch is left to speak
    model->blockSignals(true);
    for(int i = 0; i < tracks.count() - 1; ++i)
        model->setQueued(tracks.at(i));
    model->blockSignals(false);
    model->setQueued(tracks.constLast());
}

void XUiQueueDialog::clearQueue()
{
    m_manager->clearQueue(); //one queue behind every playlist
    reload();
}

void XUiQueueDialog::saveAsPlayList()
{
    if(m_entries.isEmpty())
        return;

    QInputDialog prompt(this);
    prompt.setInputMode(QInputDialog::TextInput);
    prompt.setWindowTitle(tr("New Playlist"));
    prompt.setLabelText(tr("Playlist name:"));
    prompt.setTextValue(QString());
    //QInputDialog has no placeholder of its own, so its field is asked for
    if(QLineEdit *field = prompt.findChild<QLineEdit *>())
        field->setPlaceholderText(tr("new playlist"));
    if(prompt.exec() != QDialog::Accepted)
        return;

    QString name = prompt.textValue().trimmed();
    if(name.isEmpty())
        name = tr("new playlist"); //what the empty field offered

    m_manager->createPlayList(name)->addTracks(copyOfQueue());
}
