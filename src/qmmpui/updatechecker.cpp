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

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <qmmp/qmmp.h>
#include <qmmp/qmmpsettings.h>
#include "updatechecker.h"

namespace
{
    //The repository x-AMP is published from. The releases API answers with the
    //newest one that is neither a draft nor a pre-release, which is exactly
    //the question being asked.
    const char RELEASES_API[] =
        "https://api.github.com/repos/RavilesX/x-AMP/releases/latest";
    //Where the user is sent. Built here rather than taken from the reply, so
    //the only address x-AMP can ever open is this one.
    const char RELEASES_PAGE[] = "https://github.com/RavilesX/x-AMP/releases";

    const char AUTOMATIC_KEY[] = "Updates/check_on_startup";

    //GitHub rejects unidentified clients, and a version in the agent is what
    //makes a server log useful when someone reports that the check fails.
    QByteArray userAgent()
    {
        return QByteArrayLiteral("x-AMP/") + Qmmp::strVersion().toLatin1();
    }
}

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);

    //The same global proxy the playlist downloader honours. Someone who can
    //only reach the network through a proxy would otherwise get a check that
    //always fails, with nothing saying why.
    if(QmmpSettings *gs = QmmpSettings::instance(); gs && gs->isProxyEnabled())
    {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, gs->proxy().host(),
                            quint16(gs->proxy().port()));
        if(gs->proxyType() == QmmpSettings::SOCKS5_PROXY)
            proxy.setType(QNetworkProxy::Socks5Proxy);
        if(gs->useProxyAuth())
        {
            proxy.setUser(gs->proxy().userName());
            proxy.setPassword(gs->proxy().password());
        }
        m_manager->setProxy(proxy);
    }
}

UpdateChecker::~UpdateChecker() = default;

bool UpdateChecker::isChecking() const
{
    return m_reply != nullptr;
}

bool UpdateChecker::isAutomatic()
{
    return QSettings().value(QLatin1String(AUTOMATIC_KEY), false).toBool();
}

void UpdateChecker::setAutomatic(bool enabled)
{
    QSettings().setValue(QLatin1String(AUTOMATIC_KEY), enabled);
}

int UpdateChecker::compareVersions(const QString &a, const QString &b)
{
    //Only the release part is compared. A development build calls itself
    //"1.2.0-dev", and treating that as its own version would either hide the
    //1.2.0 release from it or offer it as an upgrade to itself.
    const QStringList left = a.section(QLatin1Char('-'), 0, 0)
                              .split(QLatin1Char('.'), Qt::SkipEmptyParts);
    const QStringList right = b.section(QLatin1Char('-'), 0, 0)
                               .split(QLatin1Char('.'), Qt::SkipEmptyParts);

    for(int i = 0; i < qMax(left.count(), right.count()); ++i)
    {
        //A component that is not a number counts as zero rather than aborting
        //the comparison: a tag nobody expected should not be read as an update.
        const int l = i < left.count() ? left.at(i).toInt() : 0;
        const int r = i < right.count() ? right.at(i).toInt() : 0;
        if(l != r)
            return l < r ? -1 : 1;
    }
    return 0;
}

void UpdateChecker::check()
{
    if(m_reply)
        return; //one at a time

    QNetworkRequest request{ QUrl(QLatin1String(RELEASES_API)) };
    request.setRawHeader("User-Agent", userAgent());
    request.setRawHeader("Accept", "application/vnd.github+json");
    //Redirects are followed, but only to https. The default policy would let
    //an answer walk the request down to plain http.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(15000);

    m_reply = m_manager->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->deleteLater();
        onFinished(reply);
    });
}

void UpdateChecker::onFinished(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError)
    {
        emit checkFailed(reply->errorString());
        return;
    }

    //Capped before parsing. The endpoint answers with a few kilobytes, and a
    //reply far larger than that is not something to hand to a JSON parser.
    const QByteArray body = reply->read(256 * 1024);

    QJsonParseError error {};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if(error.error != QJsonParseError::NoError || !doc.isObject())
    {
        emit checkFailed(tr("The server sent a reply x-AMP could not read."));
        return;
    }

    //"v1.1.0" is the tag; the version inside it is what gets compared.
    QString latest = doc.object().value(QLatin1String("tag_name")).toString();
    if(latest.startsWith(QLatin1Char('v')))
        latest.remove(0, 1);
    if(latest.isEmpty())
    {
        emit checkFailed(tr("The server did not say which release is the newest."));
        return;
    }

    if(compareVersions(Qmmp::strVersion(), latest) < 0)
        emit updateAvailable(latest, QLatin1String(RELEASES_PAGE));
    else
        emit noUpdateAvailable();
}
