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

#ifndef XUIPLAYLISTCARD_H
#define XUIPLAYLISTCARD_H

#include <QImage>
#include <QPixmap>
#include <QWidget>

class QLineEdit;
class UiHelper;
class MediaPlayer;
class PlayListManager;
class XUiListView;
class XUiMenuButton;

/*!
 * The playlist card: header with add/sort/search, the track list, and the
 * row of menu buttons along the bottom.
 */
class XUiPlaylistCard : public QWidget
{
    Q_OBJECT
public:
    explicit XUiPlaylistCard(QWidget *parent = nullptr);

    /*! Shows or hides the search field. Driven by Ctrl+F on the window. */
    void toggleSearch();

    /*! The header bar, which doubles as the drag handle of its window. */
    QWidget *header() const { return m_header; }

public slots:
    /*!
     * Re-reads the background image from the settings and repaints.
     *
     * Called when the preferences page changes it; the file is only read
     * here, never in paintEvent().
     */
    void reloadBackground();

signals:
    /*! The card's own close button was pressed. */
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private slots:
    void showAddMenu();
    void showRemoveMenu();
    void showSelectMenu();
    void showPlaylistsMenu();

private:
    QWidget *buildHeader();
    QWidget *m_header = nullptr;
    QWidget *buildFooter();

    /*!
     * Renders m_backdrop at the current size as the engraving drawn behind
     * the list: the image reduced to one tone of the card's own colour, faded
     * towards the top so the track names keep their contrast.
     *
     * Costly enough to be worth caching -- it runs on a resize or on a change
     * of image, not on every paint.
     */
    void buildEngraving();
    QImage m_backdrop;   //the file as loaded, at its own size
    QPixmap m_engraving; //m_backdrop tinted and scaled to this widget

    UiHelper *m_uiHelper;
    MediaPlayer *m_player;
    PlayListManager *m_manager;
    XUiListView *m_list;
    QLineEdit *m_search;
    XUiMenuButton *m_playlists;
};

#endif
