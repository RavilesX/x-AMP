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

class QScrollBar;
class PlayListModel;
class PlayListManager;

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

private slots:
    void onModelChanged();
    void setModel(PlayListModel *model);

private:
    /*! Row index under \b y, or -1 past the end. */
    int rowAt(int y) const;
    int visibleRows() const;
    void updateScrollBar();
    void ensureVisible(int row);
    /*! Model indices currently shown, honouring the filter. */
    const QList<int> &rows() const { return m_rows; }
    void rebuildRows();

    PlayListManager *m_manager;
    PlayListModel *m_model = nullptr;
    QScrollBar *m_scrollBar;
    QList<int> m_rows;
    QString m_filter;
    int m_anchor = -1; //for shift-extended selection

    //drag to reorder
    QPoint m_pressPos;
    int m_pressRow = -1;
    int m_dropRow = -1;
    bool m_dragging = false;
};

#endif
