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

    m_scrollBar = new QScrollBar(Qt::Vertical, this);
    m_scrollBar->setSingleStep(1);
    m_scrollBar->setPageStep(6);
    m_scrollBar->setStyleSheet(QStringLiteral(
        "QScrollBar:vertical { background: transparent; width: %1px; margin: 0; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: %3px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: %4; }"
        "QScrollBar::add-line, QScrollBar::sub-line { height: 0; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }")
        .arg(SCROLLBAR_WIDTH).arg(XUi::Border.name())
        .arg(SCROLLBAR_WIDTH / 2).arg(XUi::Accent.name()));
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
        ensureVisible(m_rows.indexOf(m_lastCurrent));
    }
    update();
}

void XUiListView::rebuildRows()
{
    m_rows.clear();
    if(!m_model)
        return;
    for(int i = 0; i < m_model->trackCount(); ++i)
    {
        if(m_filter.isEmpty())
        {
            m_rows.append(i);
            continue;
        }
        PlayListTrack *track = m_model->track(i);
        if(track && track->formattedTitle(0).contains(m_filter, Qt::CaseInsensitive))
            m_rows.append(i);
    }
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

    const int index = m_rows.at(row);
    if(e->modifiers() & Qt::ShiftModifier && m_anchor >= 0)
    {
        m_model->setSelectedLines(0, m_model->trackCount() - 1, false);
        for(int r = qMin(m_anchor, row); r <= qMax(m_anchor, row); ++r)
            m_model->setSelected(m_model->track(m_rows.at(r)), true);
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
    if(m_dragging && m_dropRow >= 0 && m_model)
    {
        unsetCursor();
        //moveTracks moves the whole selection, taking the pressed row as its
        //reference; with no filter a row index is the track index
        const int to = qMin(m_dropRow, m_rows.size() - 1);
        if(to != m_pressRow)
            m_model->moveTracks(m_pressRow, to);
    }
    m_dragging = false;
    m_pressRow = -1;
    m_dropRow = -1;
    update();
}

void XUiListView::mouseDoubleClickEvent(QMouseEvent *e)
{
    const int row = rowAt(e->position().y());
    if(row < 0 || !m_model)
        return;
    m_manager->activatePlayList(m_model);
    m_model->setCurrent(m_rows.at(row));
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
        const int row = qBound(0, m_rows.indexOf(m_model->firstSelectedLine()) + step,
                               m_rows.size() - 1);
        if(!m_rows.isEmpty())
        {
            m_model->setSelectedLines(0, m_model->trackCount() - 1, false);
            m_model->setSelected(m_model->track(m_rows.at(row)), true);
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
    connect(menu.addAction(tr("Add to &Queue")), &QAction::triggered,
            m_model, &PlayListModel::addToQueue);
    menu.addSeparator();
    connect(menu.addAction(tr("&Remove Selected")), &QAction::triggered,
            m_model, &PlayListModel::removeSelected);
    connect(menu.addAction(tr("Remove &All")), &QAction::triggered,
            m_model, &PlayListModel::clear);
    menu.exec(e->globalPos());
    update();
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
        const int row = dropRowAt(e->position().y());
        const int index = row < m_rows.size() ? m_rows.at(row) : m_model->trackCount();
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
        const int index = m_rows.at(row);
        PlayListTrack *track = m_model->track(index);
        if(!track)
            continue;

        const QRectF box(0, (row - first) * ROW_HEIGHT, width(), ROW_HEIGHT);
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
            p.setPen(XUi::TextDim);
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
