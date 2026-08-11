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

#ifndef XUILISTVIEW_H
#define XUILISTVIEW_H

#include <QWidget>

class QMenu;
class QScrollBar;
class PlayListModel;
class PlayListManager;
class PlayListTrack;

/*!
 * Track list, drawn with QPainter over PlayListModel.
 *
 * Not a QAbstractItemView subclass: PlayListModel is not a QAbstractItemModel,
 * and the interesting state (current track, queue position, selection) already
 * lives in it. Painting rows directly avoids a proxy model that would only
 * mirror what the engine already tracks.
 */
class XUiListView : public QWidget
{
    Q_OBJECT
public:
    explicit XUiListView(PlayListManager *manager, QWidget *parent = nullptr);

    /*! Filters visible rows by a substring of the title. Empty shows all. */
    void setFilter(const QString &filter);

signals:
    /*! A row was activated, so the caller can start playback. */
    void activated();

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;

private slots:
    void onModelChanged();
    void setModel(PlayListModel *model);

private:
    /*! Row index under \b y, or -1 past the end. */
    int rowAt(int y) const;
    int visibleRows() const;
    void updateScrollBar();
    void ensureVisible(int row);
    /*!
     * One visible line: either a track, or a heading naming the folder the
     * tracks below it come from. Both occupy a full row, so all the geometry
     * stays a simple multiple of ROW_HEIGHT.
     */
    struct Row
    {
        int track = -1;   //-1 marks a heading
        QString folder;   //set on headings only
        bool isHeading() const { return track < 0; }
    };

    void rebuildRows();
    /*! Adds the play queue commands to \b menu as a submenu of their own. */
    void addQueueMenu(QMenu *menu);
    /*! Moves the selection ahead of the tracks already waiting in the queue. */
    void queueSelectedFirst();
    /*! Flips the queued state of \b tracks, refreshing the views only once. */
    void toggleQueued(const QList<PlayListTrack *> &tracks);
    /*! Display row showing \b trackIndex, or -1. */
    int rowForTrack(int trackIndex) const;
    /*! First track row at or after \b row, for drops and keyboard moves. */
    int trackRowFrom(int row, int step) const;

    PlayListManager *m_manager;
    PlayListModel *m_model = nullptr;
    QScrollBar *m_scrollBar;
    QList<Row> m_rows;
    QString m_filter;
    int m_anchor = -1; //for shift-extended selection

    /*! Row boundary nearest \b y, for both reordering and external drops. */
    int dropRowAt(int y) const;

    //current track the view last scrolled to, so it only follows real
    //changes of track rather than every list update
    int m_lastCurrent = -1;

    //drag to reorder
    QPoint m_pressPos;
    int m_pressRow = -1;
    int m_dropRow = -1;
    bool m_dragging = false;
};

#endif
