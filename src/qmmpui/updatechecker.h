/***************************************************************************
 *   Copyright (C) 2026 by x-AMP developers                                *
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
#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include "qmmpui_export.h"

class QNetworkAccessManager;
class QNetworkReply;

/*! @brief Asks GitHub whether a newer x-AMP has been released.
 *
 * One request to the releases API of the repository x-AMP is published from,
 * comparing the tag it reports against the running version. It reaches the
 * network only when asked: nothing here runs on a timer, so an interface that
 * never calls check() never talks to anyone.
 *
 * The reply is somebody else's data. It is parsed defensively and used for
 * two things only -- a version string to compare and a URL to open in the
 * browser -- and the URL is refused unless it is https on github.com, so a
 * compromised or spoofed answer cannot point the user somewhere else.
 */
class QMMPUI_EXPORT UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker();

    /*!
     * Sends the request. Exactly one of the signals below follows. Calling it
     * again while one is in flight is ignored.
     */
    void check();
    /*!
     * Returns \b true while a request is outstanding.
     */
    bool isChecking() const;

    /*!
     * Returns \b true when the interface should check on startup. Off unless
     * the user turns it on: a player that contacts a server the first time it
     * runs, without being asked, is not something to do by default.
     */
    static bool isAutomatic();
    /*!
     * Records the preference read by isAutomatic().
     */
    static void setAutomatic(bool enabled);

    /*!
     * Compares two dotted version strings numerically, so that 1.10.0 is
     * newer than 1.9.0 -- which a string comparison gets backwards. Missing
     * components count as zero, and anything after a dash is ignored, so
     * "1.2.0-dev" compares equal to "1.2.0". Returns a negative number when
     * \b a is older, zero when they match, positive when \b a is newer.
     */
    static int compareVersions(const QString &a, const QString &b);

signals:
    /*!
     * A release newer than the running version was found.
     * \param version Version of that release, without the leading "v".
     * \param url Page to send the user to.
     */
    void updateAvailable(const QString &version, const QString &url);
    /*!
     * The newest release is the one already running, or an older one.
     */
    void noUpdateAvailable();
    /*!
     * The check could not be completed. \b reason is already translated and
     * fit to show.
     */
    void checkFailed(const QString &reason);

private:
    void onFinished(QNetworkReply *reply);

    QNetworkAccessManager *m_manager = nullptr;
    QNetworkReply *m_reply = nullptr;
};

#endif
