/***************************************************************************
 *   Copyright (C) 2006-2026 by Ilya Kotov                                 *
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
#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include "playlistitem.h"
#include "playlisttrack.h"
#include "playlistgroup.h"
#include "qmmpui_export.h"

class PlayListModelPrivate;

/*! @brief Helper class that keeps track of a view's selected items.
 *
 * @author Vladimir Kuznetsov <vovanec@gmail.com>
 */
struct SimpleSelection
{
    /*!
     * Returns number of selected items.
     */
    inline int count()const
    {
        return bottom - top + 1;
    }
    int top = 1;                  /*!< Top of the selection   */
    int bottom = -1;              /*!< Bottom of the selection */
};
/*! @brief The PlayListModel class provides a data model for the playlist.
 *
 * @author Vladimir Kuznetsov <vovanec@gmail.com>
 * @author Ilya Kotov <forkotov02@ya.ru>
 *
 *              Playlist Structure
 *      ----------------------------------
 *     | line  | sub-line | item  | item  |
 *     | index | index    | index | type  |
 *      ----------------------------------
 *     |  0    |  0       |  0    | group |
 *     |  1    |  1       |  0    | group |
 *     |  2    |  2       |  0    | group |
 *      ----------------------------------
 *     |  3    |  0       |  0    | track |
 *     |  4    |  0       |  1    | track |
 *     |  5    |  0       |  2    | track |
 *      ----------------------------------
 *     |  6    |  0       |  1    | group |
 *     |  7    |  1       |  1    | group |
 *     |  8    |  2       |  1    | group |
 *      ----------------------------------
 *     |  9    |  0       |  3    | track |
 *     | 10    |  0       |  4    | track |
 */
class QMMPUI_EXPORT PlayListModel : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(PlayListModel)
public:
    /*!
     * Constructs a playlist model.
     * \param name Playlist name.
     * \param parent QObject parent
     */
    explicit PlayListModel(const QString &name, QObject *parent = nullptr);
    /*!
     * Object destructor.
     */
    ~PlayListModel();
    /*!
     * Returns playlist name.
     */
    QString name() const;
    /*!
     * Sets the name of the playlist to \b name.
     */
    void setName(const QString &name);
    /*!
     * Returns number of groups.
     */
    int groupCount() const;
    /*!
     * Returns number of tracks.
     */
    int trackCount() const;
    /*!
     * Returns \b true if the model contains no tracks; otherwise returns \b false.
     */
    bool isEmpty() const;
    /*!
     * Returns number of columns.
     */
    int columnCount() const;
    /*!
     * Returns the current track.
     */
    PlayListTrack *currentTrack() const;
    /*!
     * Returns the next playing track or \b nullptr if next track is unknown.
     */
    PlayListTrack *nextTrack() const;
    /*!
     * Returns index of track or group.
     * \param item track or group pointer.
     */
    int indexOf(PlayListItem *item) const;
    /*!
     * Returns the track with the index \b index or \b nullptr if track doesn't exist.
     */
    PlayListTrack *track(int index) const;
    /*!
     * Returns the group with the index \b index or \b nullptr if group doesn't exist.
     */
    PlayListGroup *group(int index) const;
    /*!
     * Returns the group which contains track \b track or \b nullptr if group doesn't exist.
     */
    PlayListGroup *group(const PlayListTrack *track) const;
    /*!
     * Returns a list of all playlist groups.
     */
    QList<PlayListGroup *> groups() const;
    /*!
     * Returns index of the current track or -1 if model is empty.
     */
    int currentIndex() const;
    /*!
     * Sets current track index.
     * Returns \b false if the track with this index doesn't exist, otherwise returns \b true
     * \param index Number of the track.
     */
    bool setCurrent(int index);
    /*!
     * Sets current track to \b track.
     * Returns \b true if success, otherwise returns \b false
     */
    bool setCurrent(PlayListTrack *track);
    /*!
     * Sets the selected state of the item to \b select
     * \param item Track or group pointer.
     * \param selected Selection state (\b true - select, \b false - unselect)
     */
    void setSelected(PlayListItem *item, bool selected = true);
    /*!
     * Sets the selected state of the list of tracks to \b select
     * \param tracks List of tracks.
     * \param selected Selection state (\b true - select, \b false - unselect)
     */
    void setSelected(const QList<PlayListTrack *> &tracks, bool selected = true);
    /*!
     * Sets the selected state of the list of items to \b select
     * \param items List of items.
     * \param selected Selection state (\b true - select, \b false - unselect)
     */
    void setSelected(const QList<PlayListItem *> &items, bool selected = true);
    /*!
     * Sets the selected state of the list of items range.
     * \param firstLine Firts line in the range.
     * \param lastLine Last line in the range.
     * \param selected Selection state (\b true - select, \b false - unselect).
     */
    void setSelectedLines(int firstLine, int lastLine, bool selected = true);
    /*!
     * Advances to the next item. Returns \b false if next iten doesn't exist,
     * otherwise returns \b true
     */
    bool next();
    /*!
     * Goes back to the previous item. Returns \b false if previous iten doesn't exist,
     * otherwise returns \b true
     */
    bool previous();
    /*!
     * Returns total line count.
     */
    int lineCount() const;
    /*!
     * Returns item at line \b lineIndex or \b nullptr if item at line \b lineIndex is not available.
     */
    PlayListItem *itemAtLine(int lineIndex) const;
    /*!
     * Returns track at line \b lineIndex or \b nullptr if track at line \b lineIndex is not available.
     */
    PlayListTrack *trackAtLine(int lineIndex) const;
    /*!
     * Returns a list of the items, starting at line \b pos
     * \param pos First item line.
     * \param count A number of items. If \b count is -1 (the default), all items from pos are returned.
     */
    QList<PlayListItem *> itemsAtLines(int pos, int count = -1) const;
    /*!
     * Finds line for item \b item. Returns -1 if line for this item is not available.
     */
    int findLine(const PlayListItem *item) const;
    /*!
     * Finds line for track with index \b trackIndex. Returns -1 if the line for this track is not available.
     */
    int findLine(int trackIndex) const;
    /*!
     * Returns sub-index of the line \b lineIndex.
     */
    int subIndexOfLine(int lineIndex) const;
    /*!
     * Returns track index of the line \b lineIndex. Returns -1 if the line does not contain track.
     */
    int trackIndexAtLine(int lineIndex) const;
    /*!
     * Returns \b true if the line \b lineIndex should be painted using alternate color. Otherwise returns \b false.
     */
    bool alternateColor(int lineIndex) const;
    /*!
     * Returns a number of lines needed to draw the group.
     */
    int linesPerGroup() const;
    /*!
     *  Moves the track at index position \b from to index position \b to.
     */
    void moveTracks(int from, int to);
    /*!
     * Returns a list of queued tracks.
     */
    QList<PlayListTrack *> queuedTracks() const;
    /*!
     * Returns \b true if play queue is empty, otherwise returns - \b false.
     */
    bool isEmptyQueue() const;
    /*!
     * Returns the number of tracks in the queue
     */
    int queueSize() const;
    /*!
     * Returns \b true if playback stops after \b track, otherwise returns \b false.
     */
    bool isStopAfter(const PlayListItem *track) const;
    /*!
     * Returns current selection(playlist can contain a lot of selections,
     * this method returns selection which \b trackIndex belongs to)
     */
    SimpleSelection getSelection(int trackIndex) const;
    /*!
     * Returns a list of the selected lines.
     */
    QList<int> selectedLines() const;
    /*!
     * Selects or unselects group or track at line \b line.
     * \param line Line number.
     * \param selected Selection state.
     */
    void setSelectedLine(int line, bool selected = true);
    /*!
     * Returns list with selected track indexes.
     */
    QList<int> selectedTrackIndexes() const;
    /*!
     * Returns list of \b PlayListTrack pointers that are selected.
     */
    QList<PlayListTrack *> selectedTracks() const;
    /*!
     * Returns a list of all \b PlayListTrack pointers.
     */
    QList<PlayListTrack *> tracks() const;
    /*!
     * Returns a number of the first line. Returns -1 if the playlist does not contain selected lines.
     */
    int firstSelectedLine() const;
    /*!
     * Returns a number of the first line. Returns -1 if the playlist does not contain selected lines.
     */
    int lastSelectedLine() const;
    /*!
     * Returns total duration in milliseconds of all songs.
     */
    qint64 totalDuration() const;
    /*!
     * Loads playlist with \b f_name name.
     */
    void loadPlaylist(const QString& f_name);
    /*!
     * Loads playlist from content.
     * \param fmt Playlist format (short name).
     * \param data Content of the playlist file.
     */
    void loadPlaylist(const QString &fmt, const QByteArray &data);
    /*!
     * Saves current songs to the playlist with \b f_name name.
     */
    void savePlaylist(const QString &f_name);
    /*!
     * Returns \b true if the file loader thread is active; otherwise returns \b false.
     */
    bool isLoaderRunning() const;
    /*!
     * Returns \b true if the playlist contains an item with URL \b url; otherwise returns \b false.
     */
    bool contains(const QString &url);
    /*!
     * Enum of the available sort modes.
     */
    enum SortMode
    {
        TITLE = 0,              /*!< by title */
        ALBUM,                  /*!< by album */
        DISCNUMBER,             /*!< by discnumber */
        ARTIST,                 /*!< by artist */
        ALBUMARTIST,            /*!< by album artist */
        FILENAME,               /*!< by file name */
        PATH_AND_FILENAME,      /*!< by path and file name */
        DATE,                   /*!< by date */
        TRACK,                  /*!< by track */
        FILE_CREATION_DATE,     /*!< by file creation date */
        FILE_MODIFICATION_DATE, /*!< by file modification date */
        GROUP                   /*!< by group name */
    };
    /*!
     * Finds track with index \b trackIndex.
     * Returns null pointer if playlist does not contain track with index \b track_index.
     */
    PlayListTrack *findTrack(int trackIndex) const;
    /*!
     * Finds tracks by string \b str. The search is case insensitive.
     * Returns a list of \b PlayListItem pointers.
     */
    QList<PlayListItem *> findTracks(const QString &str) const;
    /*!
     * Enum of the playlist update flags.
     */
    enum UpdateFlags
    {
        STRUCTURE = 0x01,  /*!< Structure of the playlist has been changed */
        SELECTION = 0x02,  /*!< Current selection has been changed */
        QUEUE = 0x04,      /*!< Track queue has been changed */
        CURRENT = 0x08,    /*!< Current track has been changed */
        STOP_AFTER = 0x10, /*!< Stop track has been changed */
        METADATA = 0x20    /*!< Metadata has been changed */
    };

signals:
    /*!
     * Emitted when the state of PlayListModel has changed.
     * \param flags Playlist updated flags. See \b UpdateFlags enum for details.
     */
    void listChanged(int flags);
    /*!
     * Emitted when new tracks have added.
     * \param tracks A list of pointers of the new playlist tracks.
     */
    void tracksAdded(const QList<PlayListTrack *> &tracks);
    /*!
     * Emitted when playlist name has chanded.
     * \param name New playlist name.
     */
    void nameChanged(const QString &name);
    /*!
     * Emitted when playlist loader thread has finished.
     */
    void loaderFinished();
    /*!
     * Tells playlist widget to show track at index \b trackIndex.
     */
    void scrollToRequest(int trackIndex);
    /*!
     * Emitted when sorting by column is finished.
     * \param column Column index.
     * \param reverted Sort direction.
     */
    void sortingByColumnFinished(int column, bool reverted);
    /*!
     * Emitted when the current track is removed.
     */
    void currentTrackRemoved();

public slots:
    /*!
     * Adds \b track to the playlist.
     */
    void addTrack(PlayListTrack *track);
    /*!
     * Adds a list of tracks to the playlist.
     * \param tracks List of tracks.
     */
    void addTracks(const QList<PlayListTrack *> &tracks);
    /*!
     * Adds a list of files and directories to the playlist
     * \param path Full path of file or directory.
     */
    void addPath(const QString &path);
    /*!
     * Adds a list of files and directories to the playlist
     * \param paths Full paths of files and directories.
     */
    void addPaths(const QStringList &paths);
    /*!
     * Inserts \b track at index position \b index in the playlist.
     */
    void insertTrack(int index, PlayListTrack *track);
    /*!
     * Inserts \b tracks at index position \b index in the playlist.
     */
    void insertTracks(int index, const QList<PlayListTrack *> &tracks);
    /*!
     * Inserts serialized content \b json at index position \b index in the playlist.
     * May be useful for fast drag-and-drop.
     */
    void insertJson(int index, const QByteArray &json);
    /*!
     * Inserts file or directory at index position \b index in the playlist.
     * \param path Full path of file or directory.
     * \param index Position in the playlist.
     */
    void insertPath(int index, const QString &path);
    /*!
     * Adds a list of files and directories at index position \b index in the playlist.
     * \param paths Full paths of files and directories.
     * \param index Position in the playlist.
     */
    void insertPaths(int index, const QStringList &paths);
    /*!
     * Adds a list of URLs at index position \b index in the playlist.
     * \param urls A list of URLs.
     * \param index Position in the playlist.
     */
    void insertUrls(int index, const QList<QUrl> &urls);
    /*!
     * Removes all items.
     */
    void clear();
    /*!
     * Clears selection.
     */
    void clearSelection();
    /*!
     * Removes selected items.
     */
    void removeSelected();
    /*!
     * Removes unselected items.
     */
    void removeUnselected();
    /*!
     * Removes track with \b i index.
     */
    void removeTrack(int i);
    /*!
     * Removes track \b track from playlist.
     */
    void removeTrack(PlayListTrack *track);
    /*!
     * Removes tracks \b items from playlist.
     */
    void removeTracks(const QList<PlayListItem *> &items);
    /*!
     * Removes tracks \b tracks from playlist.
     */
    void removeTracks(const QList<PlayListTrack *> &tracks);
    /*!
     * Inverts selection (selects unselected items and unselects selected items)
     */
    void invertSelection();
    /*!
     * Selects all items.
     */
    void selectAll();
    /*!
     * Shows details for the first selected item.
     * \param parent parent Widget.
     */
    void showDetails(QWidget *parent = nullptr);
    /*!
     * Shows the details for the current song (if any).
     * \param parent parent widget.
     */
    void showDetailsForCurrent(QWidget *parent = nullptr);
    /*!
     * Ensures that the current track is visible.
     */
    void doCurrentVisibleRequest();
    /*!
     * Ensures that the playlist track at \b trackIndex is visible.
     */
    void scrollTo(int trackIndex);
    /*!
     * Randomly changes items order.
     */
    void randomizeList();
    /*!
     * Reverces tracks order.
     */
    void reverseList();
    /*!
     * Sorts selected items in \b mode sort mode.
     */
    void sortSelection(PlayListModel::SortMode mode);
    /*!
     * Sorts items in \b mode sort mode.
     */
    void sort(PlayListModel::SortMode mode);
    /*!
     * Sorts tracks by the column with index \b column.
     */
    void sortByColumn(int column);
    /*!
     * Adds/removes selected items to/from playback queue.
     */
    void addToQueue();
    /*!
     * Adds/removes track \b t to/from playback queue.
     */
    void setQueued(PlayListTrack *t);
    /*!
     * Removes invalid tracks from playlist
     */
    void removeInvalidTracks();
    /*!
     * Removes duplicate tracks by URL.
     */
    void removeDuplicates();
    /*!
     * Removes invalid tracks and scans parent directories for the new files
     */
    void refresh();
    /*!
     * Removes all items from queue.
     */
    void clearQueue();
    /*!
     * Toggles 'stop after selected' feature.
     */
    void stopAfterSelected();
    /*!
     * Rebuilds groups
     */
    void rebuildGroups();
    /*!
     * Requires to update metadata.
     */
    void updateMetaData();

private:
    PlayListModelPrivate *d_ptr;
};

Q_DECLARE_METATYPE(PlayListModel::SortMode)

#endif
