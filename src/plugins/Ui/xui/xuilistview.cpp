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

#include <QContextMenuEvent>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QApplication>
#include <QWheelEvent>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlisttrack.h>
#include "xuitheme.h"
#include "xuiicons.h"
#include "xuilistview.h"

namespace
{
    constexpr int ROW_HEIGHT = 28;
    constexpr int PADDING = 12;
    constexpr int SCROLLBAR_WIDTH = 8;
}

XUiListView::XUiListView(PlayListManager *manager, QWidget *parent)
    : QWidget(parent), m_manager(manager)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    //keep a few rows even at the window's minimum size: a playlist card
    //showing only its header and footer reads as broken
    setMinimumHeight(ROW_HEIGHT * 3);
    setAcceptDrops(true); //files dropped from the file manager
    //an explicit cursor, so the window's edge-resize cursor is never inherited
    setCursor(Qt::ArrowCursor);

    m_scrollBar = new QScrollBar(Qt::Vertical, this);
    m_scrollBar->setSingleStep(1);
    m_scrollBar->setPageStep(6);
    //appearance comes from the window's style sheet, in XUi::styleSheet()
    connect(m_scrollBar, &QScrollBar::valueChanged, this, qOverload<>(&QWidget::update));

    connect(m_manager, &PlayListManager::selectedPlayListChanged,
            this, &XUiListView::setModel);
    setModel(m_manager->selectedPlayList());
}

void XUiListView::setModel(PlayListModel *model)
{
    if(m_model == model)
        return;
    if(m_model)
        disconnect(m_model, nullptr, this, nullptr);
    m_model = model;
    if(m_model)
    {
        connect(m_model, &PlayListModel::listChanged, this, &XUiListView::onModelChanged);
        connect(m_model, &PlayListModel::loaderFinished, this, &XUiListView::onModelChanged);
    }
    onModelChanged();
}

void XUiListView::onModelChanged()
{
    rebuildRows();
    updateScrollBar();
    //Only follow the playing track when it actually changes. listChanged also
    //fires for selection, so scrolling on every one of them yanked the view
    //back to the current track as soon as a distant row was clicked.
    if(m_model && m_model->currentIndex() != m_lastCurrent)
    {
        m_lastCurrent = m_model->currentIndex();
        ensureVisible(rowForTrack(m_lastCurrent));
    }
    update();
}

void XUiListView::rebuildRows()
{
    m_rows.clear();
    if(!m_model)
        return;

    //First pass: which tracks are shown, and the folder each one sits in.
    QList<int> tracks;
    QStringList folders;
    for(int i = 0; i < m_model->trackCount(); ++i)
    {
        PlayListTrack *track = m_model->track(i);
        if(!track)
            continue;
        if(!m_filter.isEmpty()
           && !track->formattedTitle(0).contains(m_filter, Qt::CaseInsensitive))
            continue;
        tracks.append(i);
        folders.append(QFileInfo(track->path()).absolutePath());
    }

    //Headings would be noise on a playlist that is all one folder, so they
    //only appear once there is more than one to tell apart.
    const bool heading = QSet<QString>(folders.cbegin(), folders.cend()).size() > 1;

    QString previous;
    for(int i = 0; i < tracks.size(); ++i)
    {
        if(heading && folders.at(i) != previous)
        {
            previous = folders.at(i);
            Row head;
            //the folder's own name, not the whole path: the point is to tell
            //neighbouring groups apart, not to spell out where they live
            head.folder = QDir(previous).dirName();
            if(head.folder.isEmpty())
                head.folder = previous; //filesystem root, or a bare URL
            m_rows.append(head);
        }
        Row row;
        row.track = tracks.at(i);
        m_rows.append(row);
    }
}

int XUiListView::rowForTrack(int trackIndex) const
{
    for(int i = 0; i < m_rows.size(); ++i)
    {
        if(m_rows.at(i).track == trackIndex)
            return i;
    }
    return -1;
}

int XUiListView::trackRowFrom(int row, int step) const
{
    for(int i = row; i >= 0 && i < m_rows.size(); i += step)
    {
        if(!m_rows.at(i).isHeading())
            return i;
    }
    return -1;
}

void XUiListView::setFilter(const QString &filter)
{
    if(m_filter == filter)
        return;
    m_filter = filter;
    rebuildRows();
    updateScrollBar();
    update();
}

int XUiListView::visibleRows() const
{
    return qMax(1, height() / ROW_HEIGHT);
}

void XUiListView::updateScrollBar()
{
    const int overflow = qMax(0, m_rows.size() - visibleRows());
    m_scrollBar->setRange(0, overflow);
    m_scrollBar->setPageStep(visibleRows());
    m_scrollBar->setVisible(overflow > 0);
}

void XUiListView::ensureVisible(int row)
{
    if(row < 0)
        return;
    if(row < m_scrollBar->value())
        m_scrollBar->setValue(row);
    else if(row >= m_scrollBar->value() + visibleRows())
        m_scrollBar->setValue(row - visibleRows() + 1);
}

int XUiListView::dropRowAt(int y) const
{
    //the marker sits between rows, so round to the nearest boundary
    return qBound(0, m_scrollBar->value() + (y + ROW_HEIGHT / 2) / ROW_HEIGHT,
                  m_rows.size());
}

int XUiListView::rowAt(int y) const
{
    const int row = m_scrollBar->value() + y / ROW_HEIGHT;
    return row < m_rows.size() ? row : -1;
}

void XUiListView::resizeEvent(QResizeEvent *)
{
    m_scrollBar->setGeometry(width() - SCROLLBAR_WIDTH - 4, 4,
                             SCROLLBAR_WIDTH, height() - 8);
    updateScrollBar();
}

void XUiListView::wheelEvent(QWheelEvent *e)
{
    //one notch is 120 units; scroll by however many lines the desktop asks for
    const qreal notches = e->angleDelta().y() / 120.0;
    m_scrollBar->setValue(m_scrollBar->value()
                          - qRound(notches * QApplication::wheelScrollLines()));
    e->accept();
}

void XUiListView::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    m_pressPos = e->pos();
    m_pressRow = -1;
    m_dragging = false;
    const int row = rowAt(e->position().y());
    if(row < 0 || !m_model)
    {
        if(e->button() == Qt::LeftButton)
            m_model->setSelectedLines(0, m_model->trackCount() - 1, false);
        update();
        return;
    }

    if(m_rows.at(row).isHeading())
    {
        update(); //a heading is a label, not something to select or drag
        return;
    }

    const int index = m_rows.at(row).track;
    if(e->modifiers() & Qt::ShiftModifier && m_anchor >= 0)
    {
        m_model->setSelectedLines(0, m_model->trackCount() - 1, false);
        for(int r = qMin(m_anchor, row); r <= qMax(m_anchor, row); ++r)
        {
            if(!m_rows.at(r).isHeading())
                m_model->setSelected(m_model->track(m_rows.at(r).track), true);
        }
    }
    else if(e->modifiers() & Qt::ControlModifier)
    {
        PlayListTrack *track = m_model->track(index);
        m_model->setSelected(track, !track->isSelected());
        m_anchor = row;
    }
    else
    {
        m_model->setSelectedLines(0, m_model->trackCount() - 1, false);
        m_model->setSelected(m_model->track(index), true);
        m_anchor = row;
    }
    //reordering is only offered on the unfiltered list: with rows hidden the
    //drop position would not mean what it looks like
    if(e->button() == Qt::LeftButton && m_filter.isEmpty())
        m_pressRow = row;
    update();
}

void XUiListView::mouseMoveEvent(QMouseEvent *e)
{
    if(m_pressRow < 0 || !(e->buttons() & Qt::LeftButton))
        return;
    if(!m_dragging)
    {
        if((e->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance())
            return;
        m_dragging = true;
        setCursor(Qt::ClosedHandCursor);
    }

    //scroll when dragged past either edge, so a track can be moved further
    //than one screenful
    const int y = e->position().y();
    if(y < ROW_HEIGHT)
        m_scrollBar->setValue(m_scrollBar->value() - 1);
    else if(y > height() - ROW_HEIGHT)
        m_scrollBar->setValue(m_scrollBar->value() + 1);

    m_dropRow = dropRowAt(y);
    update();
}

void XUiListView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    if(m_dragging)
        setCursor(Qt::ArrowCursor); //restore whatever the drag replaced
    if(m_dragging && m_dropRow >= 0 && m_model)
    {
        //moveTracks moves the whole selection, taking the pressed row as its
        //reference; with no filter a row index is the track index
        //moveTracks works in track indices, so map both ends off the
        //display rows, which may have headings interleaved
        const int fromRow = trackRowFrom(m_pressRow, 1);
        int toRow = trackRowFrom(qMin(m_dropRow, m_rows.size() - 1), 1);
        if(toRow < 0)
            toRow = trackRowFrom(m_rows.size() - 1, -1); //dropped past the end
        if(fromRow >= 0 && toRow >= 0 && fromRow != toRow)
            m_model->moveTracks(m_rows.at(fromRow).track, m_rows.at(toRow).track);
    }
    m_dragging = false;
    m_pressRow = -1;
    m_dropRow = -1;
    update();
}

void XUiListView::mouseDoubleClickEvent(QMouseEvent *e)
{
    const int row = rowAt(e->position().y());
    if(row < 0 || !m_model || m_rows.at(row).isHeading())
        return;
    m_manager->activatePlayList(m_model);
    m_model->setCurrent(m_rows.at(row).track);
    emit activated();
}

void XUiListView::keyPressEvent(QKeyEvent *e)
{
    if(!m_model)
        return;

    switch(e->key())
    {
    case Qt::Key_Delete:
        m_model->removeSelected();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    {
        const int first = m_model->firstSelectedLine();
        if(first >= 0)
        {
            m_manager->activatePlayList(m_model);
            m_model->setCurrent(first);
            emit activated();
        }
        break;
    }
    case Qt::Key_Up:
    case Qt::Key_Down:
    {
        const int step = e->key() == Qt::Key_Down ? 1 : -1;
        const int from = rowForTrack(m_model->firstSelectedLine());
        //step past any heading that sits between the two tracks
        const int row = trackRowFrom(from < 0 ? 0 : from + step, step);
        if(row >= 0)
        {
            m_model->setSelectedLines(0, m_model->trackCount() - 1, false);
            m_model->setSelected(m_model->track(m_rows.at(row).track), true);
            m_anchor = row;
            ensureVisible(row);
        }
        break;
    }
    default:
        QWidget::keyPressEvent(e);
        return;
    }
    update();
}

void XUiListView::contextMenuEvent(QContextMenuEvent *e)
{
    if(!m_model)
        return;

    QMenu menu(this);
    connect(menu.addAction(tr("&Play")), &QAction::triggered, this, [this] {
        const int first = m_model->firstSelectedLine();
        if(first >= 0)
        {
            m_manager->activatePlayList(m_model);
            m_model->setCurrent(first);
            emit activated();
        }
    });
    addQueueMenu(&menu);
    menu.addSeparator();
    connect(menu.addAction(tr("&Remove Selected")), &QAction::triggered,
            m_model, &PlayListModel::removeSelected);
    connect(menu.addAction(tr("Remove &All")), &QAction::triggered,
            m_model, &PlayListModel::clear);
    menu.exec(e->globalPos());
    update();
}

void XUiListView::addQueueMenu(QMenu *menu)
{
    QMenu *queueMenu = menu->addMenu(tr("&Queue"));
    const QList<PlayListTrack *> selected = m_model->selectedTracks();
    QList<PlayListTrack *> unqueued;
    for(PlayListTrack *track : selected)
    {
        if(!track->isQueued())
            unqueued << track;
    }

    //one entry rather than two: a selection is either waiting or it is not,
    //and a mixed selection is finished off by queueing what is still missing
    QAction *toggle = queueMenu->addAction(unqueued.isEmpty() ? tr("&Remove from Queue")
                                                              : tr("&Add to Queue"));
    toggle->setEnabled(!selected.isEmpty());
    connect(toggle, &QAction::triggered, this, [this, selected, unqueued] {
        toggleQueued(unqueued.isEmpty() ? selected : unqueued);
    });

    QAction *first = queueMenu->addAction(tr("Add to &Top of Queue"));
    first->setEnabled(!selected.isEmpty());
    connect(first, &QAction::triggered, this, &XUiListView::queueSelectedFirst);

    QAction *clear = queueMenu->addAction(tr("&Clear Queue"));
    clear->setEnabled(!m_model->isEmptyQueue());
    connect(clear, &QAction::triggered, m_model, &PlayListModel::clearQueue);

    queueMenu->addSeparator();
    QAction *manage = queueMenu->addAction(tr("&Manage..."));
    manage->setEnabled(false); //the queue manager itself is still to be written
}

void XUiListView::queueSelectedFirst()
{
    const QList<PlayListTrack *> selected = m_model->selectedTracks();
    if(selected.isEmpty())
        return;

    //PlayListModel only ever appends, so the queue is rebuilt with the
    //selection in front of whatever was already waiting. Note that emptying
    //it also drops a pending 'stop after track'.
    QList<PlayListTrack *> order = selected;
    const QList<PlayListTrack *> queued = m_model->queuedTracks();
    for(PlayListTrack *track : queued)
    {
        if(!order.contains(track))
            order << track;
    }

    m_model->blockSignals(true);
    m_model->clearQueue();
    m_model->blockSignals(false);
    toggleQueued(order); //nothing is queued any more, so this enqueues in order
}

void XUiListView::toggleQueued(const QList<PlayListTrack *> &tracks)
{
    if(tracks.isEmpty())
        return;

    //PlayListModel reports every single track, and each report rebuilds the
    //rows of every view; only the last step of the batch is left to speak
    m_model->blockSignals(true);
    for(int i = 0; i < tracks.count() - 1; ++i)
        m_model->setQueued(tracks.at(i));
    m_model->blockSignals(false);
    m_model->setQueued(tracks.constLast());
}

void XUiListView::dragEnterEvent(QDragEnterEvent *e)
{
    //reordering is internal, so only external URLs are of interest here
    if(e->mimeData()->hasUrls() && m_filter.isEmpty())
        e->acceptProposedAction();
}

void XUiListView::dragMoveEvent(QDragMoveEvent *e)
{
    if(!e->mimeData()->hasUrls())
        return;
    e->acceptProposedAction();
    const int row = dropRowAt(e->position().y());
    if(row != m_dropRow)
    {
        m_dropRow = row;
        m_dragging = true; //reuse the reorder marker to show where they land
        update();
    }
}

void XUiListView::dragLeaveEvent(QDragLeaveEvent *)
{
    m_dropRow = -1;
    m_dragging = false;
    update();
}

void XUiListView::dropEvent(QDropEvent *e)
{
    if(m_model && e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
        const int row = trackRowFrom(dropRowAt(e->position().y()), 1);
        const int index = row >= 0 ? m_rows.at(row).track : m_model->trackCount();
        m_model->insertUrls(index, e->mimeData()->urls());
    }
    m_dropRow = -1;
    m_dragging = false;
    update();
}

void XUiListView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    if(m_rows.isEmpty())
    {
        //Empty state: glyph over two lines of guidance. The parts are dropped
        //as height runs out -- with the equalizer shown the list can be only a
        //few rows tall, and a fixed layout would spill over the card's footer.
        const bool filtered = !m_filter.isEmpty();
        constexpr qreal GLYPH = 56.0;
        constexpr qreal LINE1 = 26.0;
        constexpr qreal LINE2 = 22.0;
        constexpr qreal GAP = 8.0;

        const bool showGlyph = height() >= GLYPH + GAP + LINE1 + LINE2;
        const bool showHint = height() >= LINE1 + LINE2;
        const qreal total = (showGlyph ? GLYPH + GAP : 0.0) + LINE1
                            + (showHint ? LINE2 : 0.0);
        qreal y = (height() - total) / 2.0;

        if(showGlyph)
        {
            XUiIcons::paint(&p, filtered ? XUiIcons::Search : XUiIcons::MusicNote,
                            QRectF(width() / 2.0 - GLYPH / 2.0, y, GLYPH, GLYPH),
                            XUi::Border);
            y += GLYPH + GAP;
        }

        QFont f = font();
        f.setPointSizeF(f.pointSizeF() * 1.1);
        p.setFont(f);
        p.setPen(XUi::TextDim);
        p.drawText(QRectF(0, y, width(), LINE1), Qt::AlignCenter,
                   filtered ? tr("No matching tracks") : tr("No tracks in playlist"));
        y += LINE1;

        if(showHint)
        {
            p.setFont(font());
            p.setPen(XUi::TextFaint);
            p.drawText(QRectF(0, y, width(), LINE2), Qt::AlignCenter,
                       filtered ? tr("Try a different search")
                                : tr("Add tracks and enjoy your music"));
        }
        return;
    }

    const int first = m_scrollBar->value();
    const int last = qMin(m_rows.size(), first + visibleRows() + 1);
    const int current = m_model ? m_model->currentIndex() : -1;
    const int textWidth = width() - 2 * PADDING - SCROLLBAR_WIDTH - 8;

    QFontMetrics metrics(font());
    for(int row = first; row < last; ++row)
    {
        const QRectF box(0, (row - first) * ROW_HEIGHT, width(), ROW_HEIGHT);

        if(m_rows.at(row).isHeading())
        {
            //Discreet: small dim caps with a hairline running to the edge, so
            //it reads as a divider rather than competing with the track names.
            const QString name = m_rows.at(row).folder;
            QFont small = font();
            small.setPointSizeF(small.pointSizeF() * 0.82);
            small.setBold(true);
            small.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
            p.setFont(small);

            const int textWidth = QFontMetrics(small).horizontalAdvance(name);
            p.setPen(XUi::TextFaint);
            p.drawText(QRectF(PADDING, box.top(), textWidth + 2, ROW_HEIGHT),
                       Qt::AlignVCenter | Qt::AlignLeft, name);

            const qreal lineY = box.center().y() + 0.5;
            const qreal lineFrom = PADDING + textWidth + 10;
            const qreal lineTo = width() - PADDING - SCROLLBAR_WIDTH;
            if(lineTo > lineFrom)
            {
                p.setPen(QPen(XUi::Border, 1));
                p.drawLine(QPointF(lineFrom, lineY), QPointF(lineTo, lineY));
            }
            p.setFont(font());
            continue;
        }

        const int index = m_rows.at(row).track;
        PlayListTrack *track = m_model->track(index);
        if(!track)
            continue;

        const bool isCurrent = index == current;

        if(track->isSelected())
        {
            p.setPen(Qt::NoPen);
            p.setBrush(XUi::Hover);
            p.drawRoundedRect(box.adjusted(6, 2, -6, -2), 7, 7);
        }
        if(isCurrent)
        {
            //accent bar down the left edge marks the playing track
            p.setPen(Qt::NoPen);
            p.setBrush(XUi::Accent);
            p.drawRoundedRect(QRectF(6, box.top() + 8, 3, ROW_HEIGHT - 16), 1.5, 1.5);
        }

        const QString duration = track->formattedDuration();
        const int durationWidth = duration.isEmpty()
                                  ? 0 : metrics.horizontalAdvance(duration) + 12;

        //queue position, when the track is queued
        QString prefix = QStringLiteral("%1.").arg(index + 1);
        if(track->isQueued())
            prefix = QStringLiteral("[%1]").arg(track->queuedIndex() + 1);
        const int prefixWidth = metrics.horizontalAdvance(QStringLiteral("000.")) + 8;

        p.setPen(track->isQueued() ? XUi::Accent : XUi::TextFaint);
        p.drawText(QRectF(PADDING, box.top(), prefixWidth, ROW_HEIGHT),
                   Qt::AlignVCenter | Qt::AlignLeft, prefix);

        p.setPen(isCurrent ? XUi::Accent : XUi::Text);
        const QString title = metrics.elidedText(track->formattedTitle(0), Qt::ElideRight,
                                                 textWidth - prefixWidth - durationWidth);
        p.drawText(QRectF(PADDING + prefixWidth, box.top(),
                          textWidth - prefixWidth - durationWidth, ROW_HEIGHT),
                   Qt::AlignVCenter | Qt::AlignLeft, title);

        if(!duration.isEmpty())
        {
            //the playing row is accented end to end; a dim duration next to an
            //accented title read as a different row
            p.setPen(isCurrent ? XUi::Accent : XUi::TextDim);
            p.drawText(QRectF(width() - PADDING - SCROLLBAR_WIDTH - durationWidth,
                              box.top(), durationWidth, ROW_HEIGHT),
                       Qt::AlignVCenter | Qt::AlignRight, duration);
        }
    }

    if(m_dragging && m_dropRow >= 0)
    {
        const qreal y = (m_dropRow - first) * ROW_HEIGHT;
        QPen pen(XUi::AccentBright, 2);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.drawLine(QPointF(PADDING, y), QPointF(width() - PADDING - SCROLLBAR_WIDTH, y));
    }
}
