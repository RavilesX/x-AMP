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

#include <QFile>
#include <QBuffer>
#include <QDBusMetaType>
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QRandomGenerator>
#include <qmmp/soundcore.h>
#include <qmmp/metadatamanager.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/qmmpuisettings.h>
#include "player2object.h"

Player2Object::Player2Object(QObject *parent) : QDBusAbstractAdaptor(parent)
{
    m_core = SoundCore::instance();
    m_player = MediaPlayer::instance();
    m_pl_manager =  m_player->playListManager();
    m_ui_settings = QmmpUiSettings::instance();
    connect(m_core, &SoundCore::trackInfoChanged, this, &Player2Object::updateId);
    connect(m_core, &SoundCore::trackInfoChanged, this, &Player2Object::emitPropertiesChanged);
    connect(m_core, &SoundCore::stateChanged, this, &Player2Object::checkState);
    connect(m_core, &SoundCore::stateChanged, this, &Player2Object::emitPropertiesChanged);
    connect(m_core, &SoundCore::volumeChanged, this, &Player2Object::emitPropertiesChanged);
    connect(m_core, &SoundCore::elapsedChanged, this, &Player2Object::checkSeeking);
    connect(m_ui_settings, &QmmpUiSettings::repeatableListChanged, this, &Player2Object::emitPropertiesChanged);
    connect(m_ui_settings, &QmmpUiSettings::repeatableTrackChanged, this, &Player2Object::emitPropertiesChanged);
    connect(m_ui_settings, &QmmpUiSettings::shuffleChanged, this, &Player2Object::emitPropertiesChanged);
    connect(m_pl_manager, &PlayListManager::currentPlayListChanged, this, &Player2Object::setModel);
    setModel(m_pl_manager->currentPlayList(), nullptr);
    updateId();
    syncProperties();
}

Player2Object::~Player2Object()
{}

bool Player2Object::canControl() const
{
    return true;
}

bool Player2Object::canGoNext() const
{
    return m_pl_manager->currentPlayList()->nextTrack() != nullptr;
}

bool Player2Object::canGoPrevious() const
{
    return m_pl_manager->currentPlayList()->currentIndex() > 0;
}

bool Player2Object::canPause() const
{
    return !m_pl_manager->currentPlayList()->isEmpty();
}
bool Player2Object::canPlay() const
{
    return !m_pl_manager->currentPlayList()->isEmpty();
}

bool Player2Object::canSeek() const
{
    return m_core->duration() > 0;
}

QString Player2Object::loopStatus() const
{
    if(m_ui_settings->isRepeatableTrack())
        return u"Track"_s;

    if(m_ui_settings->isRepeatableList())
        return u"Playlist"_s;

    return u"None"_s;
}

void Player2Object::setLoopStatus(const QString &value)
{
    if(value == "Track"_L1)
    {
        m_ui_settings->setRepeatableList(false);
        m_ui_settings->setRepeatableTrack(true);
    }
    else if(value == "Playlist"_L1)
    {
        m_ui_settings->setRepeatableList(true);
        m_ui_settings->setRepeatableTrack(false);
    }
    else
    {
        m_ui_settings->setRepeatableList(false);
        m_ui_settings->setRepeatableTrack(false);
    }
}

double Player2Object::maximumRate() const
{
    return 1.0;
}

QVariantMap Player2Object::metadata() const
{
    PlayListTrack *track = m_pl_manager->currentPlayList()->currentTrack();
    if(!track || m_core->path().isEmpty())
        return QVariantMap();
    QVariantMap map;
    TrackInfo info = m_core->trackInfo();
    map[u"mpris:length"_s] = qMax(m_core->duration() * 1000 , qint64(0));
    QString coverPath = MetaDataManager::instance()->getCoverPath(info.path());
    if(!coverPath.isEmpty())
    {
        map[u"mpris:artUrl"_s] = QUrl::fromLocalFile(coverPath).toString();
    }
    else
    {
        QImage coverImage = MetaDataManager::instance()->getCover(info.path());
        if(!coverImage.isNull())
        {
            QByteArray tmp;
            QBuffer buffer(&tmp);
            buffer.open(QIODevice::WriteOnly);
            coverImage.save(&buffer, "JPEG");
            map[u"mpris:artUrl"_s] = QStringLiteral("data:image/jpeg;base64,%1").arg(QString::fromLatin1(tmp.toBase64()));
        }
    }
    if(!info.value(Qmmp::ALBUM).isEmpty())
        map[u"xesam:album"_s] = info.value(Qmmp::ALBUM);
    if(!info.value(Qmmp::ARTIST).isEmpty())
        map[u"xesam:artist"_s] = QStringList() << info.value(Qmmp::ARTIST);
    if(!info.value(Qmmp::ALBUMARTIST).isEmpty())
        map[u"xesam:albumArtist"_s] = QStringList() << info.value(Qmmp::ALBUMARTIST);
    if(!info.value(Qmmp::COMMENT).isEmpty())
        map[u"xesam:comment"_s] = QStringList() << info.value(Qmmp::COMMENT);
    if(!info.value(Qmmp::COMPOSER).isEmpty())
        map[u"xesam:composer"_s] = QStringList() << info.value(Qmmp::COMPOSER);
    if(!info.value(Qmmp::DISCNUMBER).isEmpty())
        map[u"xesam:discNumber"_s] = info.value(Qmmp::DISCNUMBER).toInt();
    if(!info.value(Qmmp::GENRE).isEmpty())
        map[u"xesam:genre"_s] = QStringList() << info.value(Qmmp::GENRE);
    if(!info.value(Qmmp::TITLE).isEmpty())
        map[u"xesam:title"_s] = info.value(Qmmp::TITLE);
    if(!info.value(Qmmp::TRACK).isEmpty())
        map[u"xesam:trackNumber"_s] = info.value(Qmmp::TRACK).toInt();
    if(!info.value(Qmmp::YEAR).isEmpty())
        map[u"xesam:contentCreated"_s] = info.value(Qmmp::YEAR);
    map[u"mpris:trackid"_s] = QVariant::fromValue<QDBusObjectPath>(m_trackID);
    if(info.path().startsWith(QLatin1Char('/')))
        map[u"xesam:url"_s] =  QUrl::fromLocalFile(info.path()).toString();
    else
        map[u"xesam:url"_s] = info.path();
    return map;
}

double Player2Object::minimumRate() const
{
    return 1.0;
}

QString Player2Object::playbackStatus() const
{
    if(m_core->state() == Qmmp::Playing)
        return u"Playing"_s;

    if (m_core->state() == Qmmp::Paused)
        return u"Paused"_s;

    return u"Stopped"_s;
}

qlonglong Player2Object::position() const
{
    return qMax(m_core->elapsed() * 1000, qint64(0));
}

double Player2Object::rate() const
{
    return 1.0;
}

void Player2Object::setRate(double value)
{
    Q_UNUSED(value)
}

bool Player2Object::shuffle() const
{
    return m_ui_settings->isShuffle();
}

void Player2Object::setShuffle(bool value)
{
    m_ui_settings->setShuffle(value);
}

double Player2Object::volume() const
{
    return qMax(m_core->leftVolume(), m_core->rightVolume()) / 100.0;
}

void Player2Object::Player2Object::setVolume(double value)
{
    value = qBound(0.0, value ,1.0);
    m_core->setVolume(value * 100);
}

void Player2Object::Next()
{
    m_player->next();
}

void Player2Object::OpenUri(const QString &in0)
{
    QString path = in0;
    if(in0.startsWith(u"file://"_s))
    {
        path = QUrl(in0).toLocalFile ();
        if(!QFile::exists(path))
            return; //error
    }
    if(!m_pl_manager->currentPlayList()->isLoaderRunning())
    {
        m_pl_manager->selectPlayList(m_pl_manager->currentPlayList());
        connect(m_pl_manager->currentPlayList(), &PlayListModel::tracksAdded, this, &Player2Object::playTrack);
        connect(m_pl_manager->currentPlayList(), &PlayListModel::loaderFinished, this, &Player2Object::disconnectPl);
    }
    m_pl_manager->currentPlayList()->addPath(path);
}

void Player2Object::Pause()
{
    if(m_core->state() == Qmmp::Playing)
        m_player->pause();
}

void Player2Object::Play()
{
    if(m_core->state() == Qmmp::Paused)
        m_player->pause();
    else if(m_core->state() != Qmmp::Playing && m_core->state() != Qmmp::Buffering)
        m_player->play();
}

void Player2Object::PlayPause()
{
    if(m_core->state() == Qmmp::Playing || m_core->state() == Qmmp::Paused)
        m_player->pause();
    else if(m_core->state() != Qmmp::Playing && m_core->state() != Qmmp::Buffering)
        m_player->play();
}

void Player2Object::Previous()
{
    m_player->previous();
}

void Player2Object::Seek(qlonglong Offset)
{
    m_core->seek(qMax(qint64(0), m_core->elapsed() + Offset / 1000));
}
void Player2Object::SetPosition(const QDBusObjectPath &TrackId, qlonglong Position)
{
    if(m_trackID == TrackId)
        m_core->seek(Position / 1000);
    else
        qCWarning(plugin, "SetPosition() called with a invalid trackId");
}

void Player2Object::Stop()
{
    m_player->stop();
}

void Player2Object::emitPropertiesChanged()
{
    QMap<QString, QVariant> prevProps = m_props;
    syncProperties();

    QVariantMap map;
    QMap<QString, QVariant>::const_iterator it;
    for (it = m_props.constBegin(); it != m_props.constEnd(); ++it)
    {
        if(it.value() != prevProps.value(it.key()))
            map.insert(it.key(), it.value());
    }

    if(map.isEmpty())
        return;

    QDBusMessage msg = QDBusMessage::createSignal(u"/org/mpris/MediaPlayer2"_s,
                                                  u"org.freedesktop.DBus.Properties"_s, u"PropertiesChanged"_s);
    msg << u"org.mpris.MediaPlayer2.Player"_s;
    msg << map;
    msg << QStringList();
    QDBusConnection::sessionBus().send(msg);
}

void Player2Object::updateId()
{
    if(m_prev_track != m_pl_manager->currentPlayList()->currentTrack())
    {
        m_trackID = QDBusObjectPath(QStringLiteral("/org/qmmp/MediaPlayer2/Track/%1")
                                    .arg(QRandomGenerator::global()->generate()));
        m_prev_track = m_pl_manager->currentPlayList()->currentTrack();
    }
}

void Player2Object::checkState(Qmmp::State state)
{
    if(state == Qmmp::Playing)
    {
        updateId();
        m_previous_pos = 0;
    }
}

void Player2Object::checkSeeking(qint64 elapsed)
{
    if(abs(elapsed - m_previous_pos) > 2000)
    {
        emit Seeked(elapsed * 1000);
    }
    m_previous_pos = elapsed;
}

void Player2Object::playTrack(const QList<PlayListTrack *> &tracks)
{
    PlayListModel *model = qobject_cast<PlayListModel*>(sender());
    m_pl_manager->selectPlayList(model);
    m_pl_manager->activatePlayList(model);
    disconnect(m_pl_manager->currentPlayList(), &PlayListModel::tracksAdded, this, &Player2Object::playTrack);
    if(!m_pl_manager->currentPlayList()->setCurrent(tracks.constFirst()))
        return;
    m_player->stop();
    m_player->play();
}

void Player2Object::disconnectPl()
{
    disconnect(qobject_cast<PlayListModel *>(sender()), &PlayListModel::tracksAdded, this, &Player2Object::playTrack);
}

void Player2Object::setModel(PlayListModel *selected, PlayListModel *previous)
{
    if(previous)
        disconnect(previous, nullptr, this, nullptr); //disconnect previous model
    connect(selected, &PlayListModel::listChanged, this, &Player2Object::emitPropertiesChanged);
}

void Player2Object::syncProperties()
{
    m_props[u"CanGoNext"_s] = canGoNext();
    m_props[u"CanGoPrevious"_s] = canGoPrevious();
    m_props[u"CanPause"_s] = canPause();
    m_props[u"CanPlay"_s] = canPlay();
    m_props[u"CanSeek"_s] = canSeek();
    m_props[u"LoopStatus"_s] = loopStatus();
    m_props[u"MaximumRate"_s] = maximumRate();
    m_props[u"MinimumRate"_s] = minimumRate();
    m_props[u"PlaybackStatus"_s] = playbackStatus();
    m_props[u"Rate"_s] = rate();
    m_props[u"Shuffle"_s] = shuffle();
    m_props[u"Volume"_s] = volume();
    m_props[u"Metadata"_s] = metadata();
}
