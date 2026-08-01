/***************************************************************************
 *   Copyright (C) 2012-2026 by Ilya Kotov                                 *
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

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QCoreApplication>
#include <qmmp/qmmp.h>
#include <qmmp/qmmpsettings.h>
#include "playlistparser.h"
#include "playlistdownloader.h"

class PlayListDownloaderPrivate
{
    Q_DECLARE_PUBLIC(PlayListDownloader)
    Q_DECLARE_TR_FUNCTIONS(PlayListDownloader)
public:
    PlayListDownloaderPrivate(PlayListDownloader *pld) : q_ptr(pld)
    {
        Q_Q(PlayListDownloader);
        manager = new QNetworkAccessManager(q);
        q->connect(manager, &QNetworkAccessManager::finished, q, [this](QNetworkReply *reply) { readResponse(reply); });
        //load global proxy settings
        QmmpSettings *gs = QmmpSettings::instance();
        if(gs->isProxyEnabled())
        {
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, gs->proxy().host(),  gs->proxy().port());
            if(gs->proxyType() == QmmpSettings::SOCKS5_PROXY)
                proxy.setType(QNetworkProxy::Socks5Proxy);
            if(gs->useProxyAuth())
            {
                proxy.setUser(gs->proxy().userName());
                proxy.setPassword(gs->proxy().password());
            }
            manager->setProxy(proxy);
        }
    }

    void readResponse(QNetworkReply *reply)
    {
        Q_Q(PlayListDownloader);
        if(!model)
        {
            reply->deleteLater();
            return;
        }

        if(reply == downloadReply)
        {
            downloadReply = nullptr;

            if(reply->error() != QNetworkReply::NoError)
            {
                emit q->finished(false, QStringLiteral("%1 (%2)").arg(reply->errorString()).arg(reply->error()));
                reply->deleteLater();
                return;
            }

            QUrl url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if(!url.isEmpty() && url != url)
            {
                reply->deleteLater();
                qCDebug(core) << "redirect to" << url.toString();
                q->start(url, model);
                return;
            }

            QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
            qCDebug(core) << "content type:" << contentType;
            PlayListFormat *fmt = PlayListParser::findByMime(contentType);
            if(!fmt)
                fmt = PlayListParser::findByUrl(url);

            if(fmt)
            {
                model->loadPlaylist(fmt->properties().shortName, reply->readAll());
                emit q->finished(true);
            }
            else
            {
                emit q->finished(false, tr("Unsupported playlist format"));
            }

            reply->deleteLater();
        }
        else if(reply == checkReply)
        {
            checkReply = nullptr;

            if(reply->error() != QNetworkReply::NoError) //playlist is not available, simply add URL
            {
                model->addPath(url.toString());
                reply->deleteLater();
                emit q->finished(true);
                return;
            }

            QUrl url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if(!url.isEmpty() && url != url)
            {
                reply->deleteLater();
                qCDebug(core) << "redirect to" << url.toString();
                q->start(url, model);
                return;
            }

            QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
            qCDebug(core) << "content type:" << contentType;
            PlayListFormat *fmt = PlayListParser::findByMime(contentType);
            if(!fmt)
                fmt = PlayListParser::findByUrl(url);

            if(fmt)
            {
                model->loadPlaylist(fmt->properties().shortName, reply->readAll());
                emit q->finished(true);
            }
            else
            {
                model->addPath(url.toString());
                emit q->finished(true);
            }

            reply->deleteLater();
        }
        else //unknown request
        {
            reply->deleteLater();
        }
    }

private:
    PlayListDownloader *q_ptr;
    QNetworkAccessManager *manager;
    QUrl redirectUrl, url;
    QNetworkReply *downloadReply = nullptr;
    QNetworkReply *checkReply = nullptr;
    QByteArray userAgent = QStringLiteral("xamp/%1").arg(Qmmp::strVersion()).toLatin1();
    QPointer<PlayListModel> model;
};

PlayListDownloader::PlayListDownloader(QObject *parent) :
    QObject(parent),
    d_ptr(new PlayListDownloaderPrivate(this))
{}

PlayListDownloader::~PlayListDownloader()
{
    delete d_ptr;
}

void PlayListDownloader::start(const QUrl &url, PlayListModel *model)
{
    Q_D(PlayListDownloader);
    d->model = model;
    d->url = url;
    d->redirectUrl.clear();

    QNetworkRequest r;
    r.setUrl(url);
    r.setRawHeader("User-Agent", d->userAgent);

    if(PlayListParser::findByUrl(url)) //is it playlist?
    {
        d->downloadReply = d->manager->get(r); //download playlist
    }
    else
    {
        d->checkReply = d->manager->get(r); //check playlist
        connect(d->checkReply, &QNetworkReply::downloadProgress, this, [d](qint64 bytesReceived) {
            if(bytesReceived > 20480 && d->checkReply) //20к - maximum playlist size
                d->checkReply->abort();
        } );
    }
}

