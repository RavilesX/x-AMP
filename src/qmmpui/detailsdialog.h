/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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
#ifndef DETAILSDIALOG_H
#define DETAILSDIALOG_H

#include <QDialog>
#include <QList>
#include <qmmpui/playlisttrack.h>
#include "qmmpui_export.h"

class PlayListTrack;
class DetailsDialogPrivate;

/** @brief The DetailsDialog class provides dialog to show/edit metadata.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMPUI_EXPORT DetailsDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DetailsDialog);
public:
    /*!
     * Constructor.
     * \param tracks a list of tracks which should be used.
     * \param parent Parent widget.
     */
    explicit DetailsDialog(const QList<PlayListTrack *> &tracks, QWidget *parent = nullptr);
    /*!
     * Constructor.
     * \param track a track which should be used.
     * \param parent Parent widget.
     */
    explicit DetailsDialog(PlayListTrack *track, QWidget *parent = nullptr);
    /*!
     * Destructor.
     */
    ~DetailsDialog();
    /*!
     * Returns a list of the modified file paths or track URLs.
     */
    QStringList modifiedPaths() const;

signals:
    /*!
     * Emitted when closed dialog has modified file paths or track URLs.
     * \param paths A list of the modified file paths or track URLs.
     */
    void metaDataChanged(const QStringList &paths);

private:
    void closeEvent(QCloseEvent *) override;
    DetailsDialogPrivate *d_ptr;
};

#endif
