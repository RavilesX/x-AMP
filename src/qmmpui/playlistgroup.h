/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
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

#ifndef PLAYLISTGROUP_H
#define PLAYLISTGROUP_H

#include <QImage>
#include "playlisttrack.h"
#include "playlistitem.h"
#include "qmmpui_export.h"

class PlayListGroupPrivate;

/** @brief The PlayListTrack class provides a group for use with the PlayListModel class.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMPUI_EXPORT PlayListGroup : public PlayListItem
{
    Q_DECLARE_PRIVATE(PlayListGroup)
public:
    /*!
     * Constructor.
     * \param groupName Internal name of the group.
     */
    explicit PlayListGroup(const QString &groupName);
    /*!
     * Object destructor.
     */
    virtual ~PlayListGroup();
    /*!
     * Returns formatted title of the  group.
     * \param line Number of title (0 or 1).
     */
    QString formattedTitle(int line = 0) const override;
    /*!
     * Returns the list of the all avaibale formatted titles (1 or 2).
     */
    QStringList formattedTitles() const override;
    /*!
     * Returns \b true if the group contains track \b track.
     * Otherwise returns \b false.
     */
    bool contains(const PlayListTrack *track) const;
    /*!
     * Returns \b true if the group is empty.
     * Otherwise returns \b false.
     */
    bool isEmpty() const;
    /*!
     * Returns a list of tracks if the group.
     */
    QList<PlayListTrack *> tracks() const;
    /*!
     * Returns number of tracks if the group.
     */
    int count() const;
    /*!
     *  Returns formatted length of the item.
     */
    QString formattedDuration() const override;
    /*!
     *  Returns internal group name. This name is intended for grouping only.
     */
    QString groupName() const override;
    /*!
     * Returns \b true.
     */
    bool isGroup() const override;
    /*!
     * Returns path or URL of the first track of the group.
     */
    QString firstTrackPath() const;
    /*!
     * Returns \b true if the cover is available. Otherwise returns \b false.
     */
    bool isCoverLoaded() const;
    /*!
     * Returns available cover image.
     */
    QImage cover() const override;
    /*!
     * Sets cover image for all group.
     */
    void setCover(const QImage &cover);
    /*!
     * Updates all group titles.
     */
    void updateMetaData();

private:
    PlayListGroupPrivate *d_ptr;
    friend class GroupedContainer;
};

#endif // PLAYLISTGROUP_H
