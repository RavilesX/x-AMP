/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QByteArray>
#include <QCryptographicHash>
#include <QXmlStreamReader>
#include <QUrl>
#include <QUrlQuery>
#include <QElapsedTimer>
#include <QTimer>
#include <QDesktopServices>
#include <QSettings>
#include <QDateTime>
#include <qmmp/soundcore.h>
#include <qmmp/qmmpsettings.h>
#include <qmmp/qmmp.h>
#include "scrobbler.h"

#define API_KEY "d71c6f01b2ea562d7042bd5f5970041f"
#define SECRET "32d47bc0010473d40e1d38bdcff20968"

void ScrobblerResponse::parse(QIODevice *device)
{
    QXmlStreamReader reader(device);
    QStringList tags;
    while(!reader.atEnd())
    {
        reader.readNext();
        if(reader.isStartElement())
        {
            tags << reader.name().toString();
            if(tags.constLast() == "lfm"_L1)
                status = reader.attributes().value(u"status"_s).toString();
            else if(tags.constLast() == "error"_L1)
                code = reader.attributes().value(u"code"_s).toString();
        }
        else if(reader.isCharacters() && !reader.isWhitespace())
        {
            if(tags.constLast() == "token"_L1)
                token = reader.text().toString();
            else if(tags.constLast() == "error"_L1)
                error = reader.text().toString();
            if(tags.count() >= 2 && tags.at(tags.count() - 2) == "session"_L1)
            {
                if(tags.constLast() == "key"_L1)
                    key = reader.text().toString();
                else if(tags.constLast() == "name"_L1)
                    name = reader.text().toString();
                else if(tags.constLast() == "subscriber"_L1)
                    subscriber = reader.text().toString();
            }
        }
        else if(reader.isEndElement())
        {
            tags.takeLast();
        }
    }
}

Scrobbler::Scrobbler(const QString &scrobblerUrl, const QString &name, QObject *parent)
    : QObject(parent),
      m_ua(QStringLiteral("qmmp-plugins/%1").arg(Qmmp::strVersion().toLower()).toLatin1()),
      m_http(new QNetworkAccessManager(this)),
      m_core(SoundCore::instance()),
      m_time(new QElapsedTimer()),
      m_cache(new ListenCache(Qmmp::cacheDir() + u"/scrobbler_"_s + name + u".cache"_s)),
      m_scrobblerUrl(scrobblerUrl),
      m_name(name)
{

    QSettings settings;
    m_session = settings.value(u"Scrobbler/"_s + name + u"_session"_s).toString();

    connect(m_http, &QNetworkAccessManager::finished, this, &Scrobbler::processResponse);
    connect(QmmpSettings::instance(), &QmmpSettings::networkSettingsChanged, this, &Scrobbler::setupProxy);
    connect(m_core, &SoundCore::trackInfoChanged, this, &Scrobbler::updateMetaData);
    connect(m_core, &SoundCore::stateChanged, this, &Scrobbler::setState);

    setupProxy();
    m_cachedSongs = m_cache->load();

    if(!m_session.isEmpty())
    {
        submit();
        if(m_core->state() == Qmmp::Playing)
        {
            setState(Qmmp::Playing);
            updateMetaData();
        }
    }
}

Scrobbler::~Scrobbler()
{
    m_cache->save(m_cachedSongs);
    delete m_time;
    delete m_cache;

}

void Scrobbler::setState(Qmmp::State state)
{
    if(state == Qmmp::Playing && m_previousState == Qmmp::Paused)
    {
        qCDebug(plugin, "[%s]: resuming from %d seconds played", qPrintable(m_name), int(m_elapsed / 1000));
        m_time->restart();
    }
    else if(state == Qmmp::Paused)
    {
        m_elapsed += m_time->elapsed();
        qCDebug(plugin, "[%s]: pausing after %d seconds played", qPrintable(m_name), int(m_elapsed / 1000));
    }
    else if(state == Qmmp::Stopped && !m_song.metaData().isEmpty())
    {
        if(m_previousState == Qmmp::Playing)
            m_elapsed += m_time->elapsed();

        if((m_elapsed > 240000) || (m_elapsed > MIN_SONG_LENGTH && m_song.duration() == 0) ||
                (m_elapsed > int(m_song.duration() / 2) && m_song.duration() > MIN_SONG_LENGTH))
        {
            m_cachedSongs << m_song;
            m_cache->save(m_cachedSongs);
        }

        submit();
        m_song.clear();
        m_elapsed = 0;
    }
    m_previousState = state;
}

void Scrobbler::updateMetaData()
{
    TrackInfo info = m_core->trackInfo();
    if(m_core->state() != Qmmp::Playing)
        return;

    if(!m_song.metaData().isEmpty() && m_song.metaData() != info.metaData())
    {
        int elapsed = (m_elapsed + m_time->elapsed());
        if((elapsed > 240000) || (elapsed > MIN_SONG_LENGTH && m_song.duration() == 0) ||
                (elapsed > int(m_song.duration() / 2) && m_song.duration() > MIN_SONG_LENGTH))
        {
            m_cachedSongs << m_song;
            m_cache->save(m_cachedSongs);
        }

        submit();
        m_song.clear();
    }

    if(!info.value(Qmmp::TITLE).isEmpty() && !info.value(Qmmp::ARTIST).isEmpty())
    {
        m_song = SongInfo(info);
        m_song.setTimeStamp(QDateTime::currentSecsSinceEpoch());
        sendNotification(m_song);
    }
    m_time->restart();
    m_elapsed = 0;
}

void Scrobbler::processResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCWarning(plugin, "[%s]: http error: %s", qPrintable(m_name), qPrintable(reply->errorString()));
    }

    ScrobblerResponse response;
    response.parse(reply);

    QString error_code;
    if(response.status != "ok"_L1 && !response.status.isEmpty())
    {
        if(!response.error.isEmpty())
        {
            qCWarning(plugin, "[%s]: status=%s, %s-%s", qPrintable(m_name), qPrintable(response.status),
                     qPrintable(response.code), qPrintable(response.error));
            error_code = response.code;
        }
        else
            qCWarning(plugin, "[%s]: invalid content", qPrintable(m_name));
    }

    if (reply == m_submitReply)
    {
        m_submitReply = nullptr;
        if (response.status == "ok"_L1)
        {
            qCDebug(plugin, "[%s]: submited %d song(s)", qPrintable(m_name), m_submitedSongs);
            while (m_submitedSongs)
            {
                m_submitedSongs--;
                m_cachedSongs.removeFirst ();
            }
            if (!m_cachedSongs.isEmpty()) //submit remaining songs
            {
                submit();
            }
            else
            {
                m_cache->save(m_cachedSongs); // update the cache file to reflect the empty cache
                updateMetaData();
            }
        }
        else if(error_code == "9"_L1) //invalid session key
        {
            m_session.clear();
            qCWarning(plugin, "[%s]: invalid session key, scrobbling disabled", qPrintable(m_name));
        }
        else if(error_code == "11"_L1 || error_code == "16"_L1 || error_code.isEmpty()) //unavailable
        {
            QTimer::singleShot(120000, this, &Scrobbler::submit);
        }
        else
        {
            m_session.clear();
            qCWarning(plugin, "[%s]: service returned unrecoverable error, scrobbling disabled",
                     qPrintable(m_name));
        }
    }
    else if (reply == m_notificationReply)
    {
        m_notificationReply = nullptr;
        if(response.status == "ok"_L1)
        {
            qCDebug(plugin, "[%s]: Now-Playing notification done", qPrintable(m_name));
        }
        else if(error_code == "9"_L1) //invalid session key
        {
            m_session.clear();
            qCWarning(plugin, "[%s]: invalid session key, scrobbling has been disabled", qPrintable(m_name));
        }
    }
    reply->deleteLater();
}

void Scrobbler::setupProxy()
{
    QmmpSettings *gs = QmmpSettings::instance();
    if (gs->isProxyEnabled())
    {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, gs->proxy().host(),  gs->proxy().port());
        if(gs->proxyType() == QmmpSettings::SOCKS5_PROXY)
            proxy.setType(QNetworkProxy::Socks5Proxy);
        if(gs->useProxyAuth())
        {
            proxy.setUser(gs->proxy().userName());
            proxy.setPassword(gs->proxy().password());
        }
        m_http->setProxy(proxy);
    }
    else
        m_http->setProxy(QNetworkProxy::NoProxy);
}

void Scrobbler::submit()
{
    if (m_cachedSongs.isEmpty() || m_session.isEmpty() || m_submitReply)
        return;

    qCDebug(plugin, "[%s]: submit request", qPrintable(m_name));
    m_submitedSongs = qMin(m_cachedSongs.size(),25);

    QMap<QString, QString> params;
    for (int i = 0; i < m_submitedSongs; ++i)
    {
        SongInfo info = m_cachedSongs[i];
        params.insert(QStringLiteral("track[%1]").arg(i), info.value(Qmmp::TITLE));
        params.insert(QStringLiteral("timestamp[%1]").arg(i), QString::number(info.timeStamp()));
        params.insert(QStringLiteral("artist[%1]").arg(i), info.value(Qmmp::ARTIST));
        params.insert(QStringLiteral("album[%1]").arg(i), info.value(Qmmp::ALBUM));
        params.insert(QStringLiteral("trackNumber[%1]").arg(i), info.value(Qmmp::TRACK));
        if(info.duration() > 0)
            params.insert(QStringLiteral("duration[%1]").arg(i), QString::number(info.duration() / 1000));
    }
    params.insert(u"api_key"_s, QStringLiteral(API_KEY));
    params.insert(u"method"_s, u"track.scrobble"_s);
    params.insert(u"sk"_s, m_session);

    const QStringList keys = params.keys();
    for(const QString &key : std::as_const(keys)) //removes empty keys
    {
        if(params.value(key).isEmpty() || params.value(key) == "0"_L1)
            params.remove(key);
    }

    QUrl url(m_scrobblerUrl);
    url.setPort(m_scrobblerUrl.startsWith(u"https"_s) ? 443 : 80);

    QUrlQuery body((QString()));
    QByteArray data;
    for(const QString &key : params.keys())
    {
        body.addQueryItem(key, params.value(key));
        data.append(key.toUtf8() + params.value(key).toUtf8());
    }
    data.append(SECRET);
    body.addQueryItem(u"api_sig"_s, QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex()));
    QByteArray bodyData = body.query(QUrl::FullyEncoded).toUtf8();
    bodyData.replace("+", QUrl::toPercentEncoding(u"+"_s));

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_ua);
    request.setRawHeader("Host", url.host().toLatin1());
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader, bodyData.size());
    m_submitReply = m_http->post(request, bodyData);
}

void Scrobbler::sendNotification(const SongInfo &info)
{
    if(m_session.isEmpty() || m_notificationReply)
        return;
    qCDebug(plugin, "[%s]: sending notification", qPrintable(m_name));

    QMap<QString, QString> params;
    params.insert(u"track"_s, info.value(Qmmp::TITLE));
    params.insert(u"artist"_s, info.value(Qmmp::ARTIST));
    if(!info.value(Qmmp::ALBUM).isEmpty())
        params.insert(u"album"_s, info.value(Qmmp::ALBUM));
    if(!info.value(Qmmp::TRACK).isEmpty())
        params.insert(u"trackNumber"_s, info.value(Qmmp::TRACK));
    if(info.duration() > 0)
        params.insert(u"duration"_s, QString::number(info.duration() / 1000));
    params.insert(u"api_key"_s, QStringLiteral(API_KEY));
    params.insert(u"method"_s, u"track.updateNowPlaying"_s);
    params.insert(u"sk"_s, m_session);

    const QStringList keys = params.keys();
    for(const QString &key : std::as_const(keys)) //removes empty keys
    {
        if(params.value(key).isEmpty())
            params.remove(key);
    }

    QUrl url(m_scrobblerUrl);
    url.setPort(m_scrobblerUrl.startsWith(u"https"_s) ? 443 : 80);

    QUrlQuery body((QString()));
    QByteArray data;
    for(const QString &key : params.keys())
    {
        body.addQueryItem(key, params.value(key));
        data.append(key.toUtf8() + params.value(key).toUtf8());
    }
    data.append(SECRET);
    body.addQueryItem(u"api_sig"_s, QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex()));
    QByteArray bodyData = body.query(QUrl::FullyEncoded).toUtf8();
    bodyData.replace("+", QUrl::toPercentEncoding(u"+"_s));

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_ua);
    request.setRawHeader("Host", url.host().toLatin1());
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader,  bodyData.size());
    m_notificationReply = m_http->post(request, bodyData);
}

ScrobblerAuth::ScrobblerAuth(const QString &scrobblerUrl, const QString &authUrl,
                             const QString &name, QObject *parent) : QObject(parent),
    m_ua(QStringLiteral("qmmp-plugins/%1").arg(Qmmp::strVersion().toLower()).toLatin1()),
    m_http(new QNetworkAccessManager(this)),
    m_scrobblerUrl(scrobblerUrl),
    m_authUrl(authUrl),
    m_name(name)
{
    connect(m_http, &QNetworkAccessManager::finished, this, &ScrobblerAuth::processResponse);

    QmmpSettings *gs = QmmpSettings::instance();
    if (gs->isProxyEnabled())
    {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, gs->proxy().host(),  gs->proxy().port());
        if(gs->proxyType() == QmmpSettings::SOCKS5_PROXY)
            proxy.setType(QNetworkProxy::Socks5Proxy);
        if(gs->useProxyAuth())
        {
            proxy.setUser(gs->proxy().userName());
            proxy.setPassword(gs->proxy().password());
        }
        m_http->setProxy(proxy);
    }
    else
        m_http->setProxy(QNetworkProxy::NoProxy);
}

void ScrobblerAuth::getToken()
{
    qCDebug(plugin, "[%s]: new token request", qPrintable(m_name));
    m_session.clear();
    QUrl url(m_scrobblerUrl + QLatin1Char('?'));
    url.setPort(m_scrobblerUrl.startsWith(u"https"_s) ? 443 : 80);
    QUrlQuery q;
    q.addQueryItem(u"method"_s, u"auth.getToken"_s);
    q.addQueryItem(u"api_key"_s, QStringLiteral(API_KEY));

    QByteArray data;
    data.append("api_key" API_KEY);
    data.append("methodauth.getToken");
    data.append(SECRET);
    q.addQueryItem(u"api_sig"_s, QString::fromLatin1(QCryptographicHash::hash(data,QCryptographicHash::Md5).toHex()));
    url.setQuery(q);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent",  m_ua);
    request.setRawHeader("Host",url.host().toLatin1());
    request.setRawHeader("Accept", "*/*");
    m_getTokenReply = m_http->get(request);
}

void ScrobblerAuth::getSession()
{
    qCDebug(plugin, "[%s]: new session request", qPrintable(m_name));
    QUrl url(m_scrobblerUrl + QLatin1Char('?'));
    url.setPort(m_scrobblerUrl.startsWith(u"https"_s) ? 443 : 80);
    QUrlQuery q;
    q.addQueryItem(u"api_key"_s, QStringLiteral(API_KEY));
    q.addQueryItem(u"method"_s, u"auth.getSession"_s);
    q.addQueryItem(u"token"_s, m_token);

    QByteArray data;
    data.append("api_key" API_KEY);
    data.append("methodauth.getSession");
    data.append("token" + m_token.toUtf8());
    data.append(SECRET);
    q.addQueryItem(u"api_sig"_s, QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex()));
    url.setQuery(q);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent",  m_ua);
    request.setRawHeader("Host",url.host().toLatin1());
    request.setRawHeader("Accept", "*/*");
    m_getSessionReply = m_http->get(request);
}

void ScrobblerAuth::checkSession(const QString &session)
{
    qCDebug(plugin, "[%s]: checking session...", qPrintable(m_name));
    m_session = session;
    QMap<QString, QString> params;
    params.insert(u"api_key"_s, QStringLiteral(API_KEY));
    params.insert(u"sk"_s, session);
    params.insert(u"method"_s, u"user.getInfo"_s);

    QUrl url(m_scrobblerUrl);
    url.setPort(m_scrobblerUrl.startsWith(u"https"_s) ? 443 : 80);

    QUrlQuery body((QString()));
    QByteArray data;
    for(const QString &key : params.keys())
    {
        body.addQueryItem(key, params.value(key));
        data.append(key.toUtf8() + params.value(key).toUtf8());
    }
    data.append(SECRET);
    body.addQueryItem(u"api_sig"_s, QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex()));
    QByteArray bodyData = body.query(QUrl::FullyEncoded).toUtf8();
    bodyData.replace("+", QUrl::toPercentEncoding(u"+"_s));

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_ua);
    request.setRawHeader("Host", url.host().toLatin1());
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader,  bodyData.size());
    m_checkSessionReply = m_http->post(request, bodyData);
}

QString ScrobblerAuth::session() const
{
    return m_session;
}

void ScrobblerAuth::processResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qCWarning(plugin, "[%s]: http error: %s", qPrintable(m_name), qPrintable(reply->errorString()));
    }

    ScrobblerResponse response;
    response.parse(reply);

    QString error_code;
    if(response.status != "ok"_L1 && !response.status.isEmpty())
    {
        if(!response.error.isEmpty())
        {
            qCWarning(plugin, "[%s]: status=%s, %s-%s", qPrintable(m_name), qPrintable(response.status),
                     qPrintable(response.code), qPrintable(response.error));
            error_code = response.code;
        }
        else
            qCWarning(plugin, "[%s]: invalid content", qPrintable(m_name));
    }

    if (reply == m_getTokenReply)
    {
        m_getTokenReply = nullptr;
        if(response.status == "ok"_L1)
        {
            m_token = response.token;
            qCDebug(plugin, "[%s]: token: %s", qPrintable(m_name), qPrintable(m_token));
            QDesktopServices::openUrl(m_authUrl + QStringLiteral("?api_key=" API_KEY "&token=") + m_token);
            emit tokenRequestFinished(NO_ERROR);
        }
        else if(error_code.isEmpty())
        {
             m_token.clear();
             emit tokenRequestFinished(NETWORK_ERROR);
        }
        else if(error_code == "8"_L1 || error_code == "7"_L1 ||  error_code == "11"_L1 || error_code.isEmpty())
        {
            m_token.clear();
            emit tokenRequestFinished(LASTFM_ERROR);
        }
        else
        {
            m_token.clear();
            emit tokenRequestFinished(LASTFM_ERROR);
        }
    }
    else if(reply == m_getSessionReply)
    {
        m_getSessionReply = nullptr;
        m_session.clear();
        if(response.status == "ok"_L1)
        {
            m_session = response.key;
            qCDebug(plugin, "[%s]: name: %s", qPrintable(m_name), qPrintable(response.name));
            qCDebug(plugin, "[%s]: key: %s", qPrintable(m_name), qPrintable(m_session));
            qCDebug(plugin, "[%s]: subscriber: %s", qPrintable(m_name), qPrintable(response.subscriber));
            emit sessionRequestFinished(NO_ERROR);
        }
        else if(error_code == "4"_L1 || error_code == "15"_L1) //invalid token
        {
            m_token.clear();
            emit sessionRequestFinished(LASTFM_ERROR);
        }
        else if(error_code == "11"_L1) //service offline
        {
            m_token.clear();
            emit sessionRequestFinished(LASTFM_ERROR);
        }
        else if(error_code == "14"_L1) // unauthorized token
        {
            m_token.clear();
            emit sessionRequestFinished(LASTFM_ERROR);
        }
        else if (error_code.isEmpty()) //network error
        {
            m_token.clear();
            emit sessionRequestFinished(NETWORK_ERROR);
        }
        else
        {
            m_token.clear();
            emit sessionRequestFinished(LASTFM_ERROR);
        }
    }
    else if(reply == m_checkSessionReply)
    {
        m_checkSessionReply = nullptr;
        if(response.status == "ok"_L1)
        {
            qCDebug(plugin, "[%s]: session ok", qPrintable(m_name));
            emit checkSessionFinished(NO_ERROR);
        }
        else if(error_code.isEmpty())
        {
            qCWarning(plugin, "[%s]: network error", qPrintable(m_name));
            emit checkSessionFinished(NETWORK_ERROR);
        }
        else
        {
            qCWarning(plugin, "[%s]: received last.fm error (code=%s)",
                     qPrintable(m_name), qPrintable(error_code));
            emit checkSessionFinished(LASTFM_ERROR);
        }
    }
    reply->deleteLater();
}
