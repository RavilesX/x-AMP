/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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
#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QMap>
#include <QImage>
#include <qmmp/qmmp.h>
#include "qmmpui_export.h"

class PlayListItemPrivate;

/** @brief The PlayListItem class provides an item for use with the PlayListModel class.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMPUI_EXPORT PlayListItem
{
    Q_DECLARE_PRIVATE(PlayListItem)
public:
    /*!
     * Constructs an empty plalist item.
     */
    PlayListItem();
    /*!
     * Object destructor.
     */
    virtual ~PlayListItem();
    /*!
     * Sets item selection flag to \b select
     * \param select State of selection (\b true select, \b false unselect)
     */
    void setSelected(bool select);
    /*!
     * Return \b true if item is selected, otherwise returns \b false.
     */
    bool isSelected() const;
    /*!
     * Returns formatted title of the item.
     * \param column Number of column.
     */
    virtual QString formattedTitle(int column) const = 0;
    /*!
     * Returns the list of the formatted titles for all columns.
     * Group separators contain only one or two titles.
     */
    virtual QStringList formattedTitles() const = 0;
    /*!
     *  Returns formatted length of the item.
     */
    virtual QString formattedDuration() const = 0;
    /*!
     * Returns the group name, intended for grouping (not for display).
     */
    virtual QString groupName() const = 0;
    /*!
     * Returns \b true if the \b PlayListItem is group separator. Otherwise returns \b false.
     */
    virtual bool isGroup() const = 0;
    /*!
     * Returns the index of the track.
     * Default implementation returns -1.
     */
    virtual int trackIndex() const;
    /*!
     * Returns available cover image.
     */
    virtual QImage cover() const;

private:
    PlayListItemPrivate *d_ptr;
};

#endif
