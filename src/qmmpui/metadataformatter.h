/***************************************************************************
 *   Copyright (C) 2015-2026 by Ilya Kotov                                 *
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

#ifndef METADATAFORMATTER_H
#define METADATAFORMATTER_H

#include <QString>
#include <QHash>
#include <QList>
#include <qmmpui/playlisttrack.h>
#include <qmmp/qmmp.h>
#include "qmmpui_export.h"

class MetaDataFormatterPrivate;

/*! @brief The MetaDataFormatter formats metadata using templates.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMPUI_EXPORT MetaDataFormatter
{
    Q_DECLARE_PRIVATE(MetaDataFormatter)
public:
    /*!
     * Constructor.
     * \param pattern Metadata template.
     * Syntax:
     * %p - artist,
     * %a - album,
     * %aa - album artist,
     * %t - title,
     * %n - track number,
     * %NN - 2-digit track number,
     * %g - genre,
     * %c - comment,
     * %C - composer,
     * %D - disc number,
     * %f - file name,
     * %F - full path,
     * %y - year,
     * %l - duration,
     * %I - track index,
     * %{bitrate} - bitrate,
     * %{samplerate} - sample rate,
     * %{channels} - number of channels,
     * %{samplesize} - bits per sample,
     * %{format} - format name,
     * %{decoder} - decoder name,
     * %{filesize} - file size,
     * %if(A,B,C) or %if(A&B&C,D,E) - condition,
     * %dir(n) - name of the directory located on \b n levels above,
     * %dir - full path of the parent directory.
     */
    MetaDataFormatter(const QString &pattern = QString());
    /*!
     * Destructor.
     */
    ~MetaDataFormatter();
    /*!
     * Setups metadata template.
     * \param pattern Metadata template string.
     */
    void setPattern(const QString &pattern);
    /*!
     * Returns metadata template.
     */
    QString pattern() const;
    /*!
     * Converts metadata of track \b track to one string using template.
     */
    QString format(const PlayListTrack *track) const;
    /*!
     * Converts metadata of \b TrackInfo reference \b info to one string using template.
     * \param info pointer to \b TrackInfo object.
     * \param trackIndex Index of track.
     */
    QString format(const TrackInfo &info, int trackIndex = 0) const;
    /*!
     * Converts metadata of \b TrackInfo pointer \b info to one string using template.
     * \param info pointer to \b TrackInfo object.
     * \param trackIndex Index of track.
     */
    QString format(const TrackInfo *info, int trackIndex = 0) const;
    /*!
     * Returns formatted duration (example: 05:02:03).
     * \param duration Duration in milliseconds.
     * \param hideZero Setting for zero values output.
     * If \b hideZero is \b true, then the function outputs empty string for zero length,
     * otherwise outputs "0:00".
     * \param showMs Adds milliseconds to the end of output (example: 05:02:03.324).
     */
    static QString formatDuration(qint64 duration, bool hideZero = true, bool showMs = false);

private:
    MetaDataFormatterPrivate *d_ptr;
};

#endif // METADATAFORMATTER_H
