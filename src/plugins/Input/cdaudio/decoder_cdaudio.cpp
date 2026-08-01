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


#include <QObject>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <cdio/cdio.h>
#include <cdio/paranoia/cdda.h>
#include <cdio/audio.h>
#include <cdio/cd_types.h>
#include <cdio/logging.h>
#ifdef WITH_LIBCDDB
#include <cddb/cddb.h>
#endif
#include <qmmp/buffer.h>
#include <qmmp/output.h>
#include <qmmp/qmmpsettings.h>

#define CDDA_SECTORS 4
#define CDDA_BUFFER_SIZE (CDDA_SECTORS*CDIO_CD_FRAMESIZE_RAW)

#include "decoder_cdaudio.h"

QList<CDATrack> DecoderCDAudio::m_track_cache;

static void log_handler (cdio_log_level_t level, const char *message)
{
    QString str = QString::fromLocal8Bit(message).trimmed();
    switch (level)
    {
    case CDIO_LOG_DEBUG:
        qCDebug(plugin, "cdio message: %s (level=debug)", qPrintable(str));
        return;
    case CDIO_LOG_INFO:
        qCDebug(plugin, "cdio message: %s (level=info)", qPrintable(str));
        return;
    default:
        qCWarning(plugin, "cdio message: %s (level=error)", qPrintable(str));
    }
}

#ifdef WITH_LIBCDDB
static void cddb_log_handler(cddb_log_level_t level, const char *message)
{
    QString str = QString::fromLocal8Bit(message).trimmed();
    switch (level)
    {
    case CDDB_LOG_DEBUG:
        qCDebug(plugin, "cddb message: %s (level=debug)", qPrintable(str));
        return;
    case CDDB_LOG_INFO:
        qCDebug(plugin, "cddb message: %s (level=info)", qPrintable(str));
        return;
    default:
        qCWarning(plugin, "cddb message: %s (level=error)", qPrintable(str));
    }
}
#endif

// Decoder class

DecoderCDAudio::DecoderCDAudio(const QString &url) : Decoder(),
    m_url(url),
    m_buffer(new char[CDDA_BUFFER_SIZE])
{}

DecoderCDAudio::~DecoderCDAudio()
{
    m_bitrate = 0;
    if (m_cdio)
    {
        cdio_destroy(m_cdio);
        m_cdio = nullptr;
    }
    delete [] m_buffer;
}

QList<CDATrack> DecoderCDAudio::generateTrackList(const QString &device, TrackInfo::Parts parts)
{
    //read settings
    QSettings settings;
    int cd_speed = settings.value(u"cdaudio/speed"_s, 0).toInt();
    bool use_cd_text = settings.value(u"cdaudio/cdtext"_s, true).toBool();
    QList<CDATrack> tracks;
    cdio_log_set_handler(log_handler); //setup cdio log handler
    CdIo_t *cdio = nullptr;
    QString device_path = device;
    if (device_path.isEmpty() || device_path == "/"_L1)
        device_path = settings.value(u"cdaudio/device"_s).toString();
    if (device_path.isEmpty() || device_path == "/"_L1)
    {
        char **cd_drives = cdio_get_devices_with_cap(nullptr, CDIO_FS_AUDIO, true); //get drive list with CDA disks
        // open first audio capable cd drive
        if (cd_drives && *cd_drives)
        {
            cdio = cdio_open_cd(*cd_drives);
            if (!cdio)
            {
                qCWarning(plugin, "failed to open CD.");
                cdio_free_device_list(cd_drives);
                return tracks;
            }
            qCDebug(plugin, "found cd audio capable drive \"%s\"", *cd_drives);
            device_path = QString::fromLatin1(*cd_drives);
            cdio_free_device_list(cd_drives); //free device list
        }
        else
        {
            qCWarning(plugin, "unable to find cd audio drive.");
            cdio_free_device_list(cd_drives);
            return tracks;
        }
    }
    else
    {
        cdio = cdio_open_cd(device_path.toLatin1().constData());
        if (!cdio)
        {
            qCWarning(plugin, "failed to open CD.");
            return tracks;
        }
        qCDebug(plugin, "using cd audio capable drive \"%s\"", qPrintable(device_path));
    }

    if(!m_track_cache.isEmpty() && !cdio_get_media_changed(cdio))
    {
        qCDebug(plugin, "using track cache...");
        cdio_destroy(cdio);
        return m_track_cache;
    }

    if (cd_speed)
    {
        qCDebug(plugin, "setting drive speed to %dX.", cd_speed);
        if (cdio_set_speed(cdio, 1) != DRIVER_OP_SUCCESS)
            qCWarning(plugin, "unable to set drive speed to %dX.", cd_speed);
    }

    cdrom_drive_t *pcdrom_drive = cdio_cddap_identify_cdio(cdio, 1, nullptr); //create paranoya CD-ROM object
    //get first and last track numbers
    int first_track_number = cdio_get_first_track_num(pcdrom_drive->p_cdio);
    int last_track_number = cdio_get_last_track_num(pcdrom_drive->p_cdio);

    if ((first_track_number == CDIO_INVALID_TRACK) || (last_track_number == CDIO_INVALID_TRACK))
    {
        qCWarning(plugin, "invalid first (last) track number.");
        cdio_destroy(cdio);
        cdio = nullptr;
        return tracks;
    }
#ifdef WITH_LIBCDDB
    bool use_cddb = true;
#endif
    //fill track list
    for (int i = first_track_number; i <= last_track_number; ++i)
    {
        CDATrack t;
        t.first_sector = cdio_get_track_lsn(pcdrom_drive->p_cdio, i);
        t.last_sector = cdio_get_track_last_lsn(pcdrom_drive->p_cdio, i);
        t.info.setDuration((t.last_sector - t.first_sector +1) * 1000 / 75);
        t.info.setValue(Qmmp::TRACK, i);
        t.info.setPath(QStringLiteral("cdda://%1#%2").arg(device_path).arg(i));

        if(parts & TrackInfo::Properties)
        {
            t.info.setValue(Qmmp::BITRATE, 1411);
            t.info.setValue(Qmmp::SAMPLERATE, 44100);
            t.info.setValue(Qmmp::CHANNELS, 2);
            t.info.setValue(Qmmp::BITS_PER_SAMPLE, 16);
            t.info.setValue(Qmmp::FORMAT_NAME, "CDDA");
        }

        if ((t.first_sector == CDIO_INVALID_LSN) || (t.last_sector== CDIO_INVALID_LSN))
        {
            qCWarning(plugin, "invalid stard(end) lsn for the track %d.", i);
            tracks.clear();
            cdio_destroy(cdio);
            cdio = nullptr;
            return tracks;
        }
        //cd text
        cdtext_t *cdtext = use_cd_text ? cdio_get_cdtext(pcdrom_drive->p_cdio) : nullptr;
        if (cdtext)
        {
            t.info.setValue(Qmmp::TITLE, QString::fromUtf8(cdtext_get_const(cdtext,CDTEXT_FIELD_TITLE,i)));
            t.info.setValue(Qmmp::ARTIST, QString::fromUtf8(cdtext_get_const(cdtext,CDTEXT_FIELD_PERFORMER,i)));
            t.info.setValue(Qmmp::GENRE, QString::fromUtf8(cdtext_get_const(cdtext,CDTEXT_FIELD_GENRE,i)));
            t.info.setValue(Qmmp::COMMENT, QString::fromUtf8(cdtext_get_const(cdtext,CDTEXT_FIELD_MESSAGE,i)));
            t.info.setValue(Qmmp::COMPOSER, QString::fromUtf8(cdtext_get_const(cdtext,CDTEXT_FIELD_COMPOSER,i)));
#ifdef WITH_LIBCDDB
            use_cddb = false;
#endif
        }
        else
            t.info.setValue(Qmmp::TITLE, QStringLiteral("CDA Track %1").arg(i, 2, 10, QLatin1Char('0')));
        tracks  << t;
    }
    qCDebug(plugin) << "found" << tracks.size() << "audio tracks";

#ifdef WITH_LIBCDDB
    use_cddb = use_cddb && settings.value(u"cdaudio/use_cddb"_s, false).toBool();
    if(use_cddb)
    {
        qCDebug(plugin, "reading CDDB...");
        cddb_log_set_handler(cddb_log_handler);
        cddb_conn_t *cddb_conn = cddb_new ();
        cddb_disc_t *cddb_disc = nullptr;
        lba_t lba;
        if (!cddb_conn)
           qCWarning(plugin) << "unable to create cddb connection";
        else
        {
            cddb_cache_disable(cddb_conn); //disable libcddb cache, use own cache implementation instead
            settings.beginGroup(u"cdaudio"_s);
            cddb_set_server_name(cddb_conn, settings.value(u"cddb_server"_s, u"gnudb.org"_s).toByteArray().constData());
            cddb_set_server_port(cddb_conn, settings.value(u"cddb_port"_s, 8880).toInt());

            if (settings.value(u"cddb_http"_s, false).toBool())
            {
                cddb_http_enable (cddb_conn);
                cddb_set_http_path_query (cddb_conn, settings.value(u"cddb_path"_s).toByteArray().constData());
                if (QmmpSettings::instance()->isProxyEnabled() && QmmpSettings::instance()->proxyType() == QmmpSettings::HTTP_PROXY)
                {
                    QUrl proxy = QmmpSettings::instance()->proxy();
                    cddb_http_proxy_enable(cddb_conn);
                    cddb_set_http_proxy_server_name(cddb_conn, proxy.host().toLatin1().constData());
                    cddb_set_http_proxy_server_port(cddb_conn, proxy.port());
                    if(QmmpSettings::instance()->useProxyAuth())
                    {
                        cddb_set_http_proxy_username(cddb_conn, proxy.userName().toLatin1().constData());
                        cddb_set_http_proxy_password(cddb_conn, proxy.password().toLatin1().constData());
                    }
                }
            }
            settings.endGroup();

            cddb_disc = cddb_disc_new ();
            lba = cdio_get_track_lba (cdio, CDIO_CDROM_LEADOUT_TRACK);
            cddb_disc_set_length (cddb_disc, FRAMES_TO_SECONDS (lba));

            for (int i = first_track_number; i <= last_track_number; ++i)
            {
                cddb_track_t *cddb_track = cddb_track_new ();
                cddb_track_set_frame_offset (cddb_track, cdio_get_track_lba (cdio, i));
                cddb_disc_add_track (cddb_disc, cddb_track);
            }

            cddb_disc_calc_discid (cddb_disc);
            uint id = cddb_disc_get_discid (cddb_disc);
            qCDebug(plugin, "disc id = %x", id);


            if(readFromCache(&tracks, id))
                qCDebug(plugin, "using local cddb cache");
            else
            {
                int matches = cddb_query (cddb_conn, cddb_disc);
                if(matches == -1)
                {
                    qCWarning(plugin, "unable to query the CDDB server, error: %s",
                             cddb_error_str (cddb_errno(cddb_conn)));
                }
                else if(matches == 0)
                {
                    qCDebug(plugin, "no CDDB info found");
                }
                else if(cddb_read(cddb_conn, cddb_disc))
                {
                    for (int i = first_track_number; i <= last_track_number; ++i)
                    {
                        cddb_track_t *cddb_track = cddb_disc_get_track (cddb_disc, i - 1);
                        int t = i - first_track_number;
                        tracks[t].info.setValue(Qmmp::ARTIST, QString::fromUtf8(cddb_track_get_artist(cddb_track)));
                        tracks[t].info.setValue(Qmmp::TITLE, QString::fromUtf8(cddb_track_get_title(cddb_track)));
                        tracks[t].info.setValue(Qmmp::GENRE, QString::fromUtf8(cddb_disc_get_genre(cddb_disc)));
                        tracks[t].info.setValue(Qmmp::ALBUM, QString::fromUtf8(cddb_disc_get_title(cddb_disc)));
                        tracks[t].info.setValue(Qmmp::YEAR, cddb_disc_get_year(cddb_disc));
                    }
                    saveToCache(tracks,  id);
                }
                else
                {
                    qCWarning(plugin, "unable to read the CDDB info: %s",
                              cddb_error_str(cddb_errno(cddb_conn)));
                }
            }
        }
        if (cddb_disc)
            cddb_disc_destroy (cddb_disc);

        if (cddb_conn)
            cddb_destroy (cddb_conn);
    }
#endif

    cdio_destroy(cdio);
    cdio = nullptr;
    m_track_cache = tracks;
    return tracks;
}

void DecoderCDAudio::saveToCache(QList<CDATrack> tracks,  uint disc_id)
{
    QDir dir(Qmmp::cacheDir());
    if(!dir.exists(u"cddbcache"_s))
        dir.mkdir(u"cddbcache"_s);
    dir.cd(u"cddbcache"_s);
    QString path = QStringLiteral("%1/%2").arg( dir.absolutePath()).arg(disc_id, 0, 16);
    QSettings settings(path, QSettings::IniFormat);
    settings.clear();
    settings.setValue(u"count"_s, tracks.size());
    for(int i = 0; i < tracks.size(); ++i)
    {
        CDATrack track = tracks[i];
        QMap<Qmmp::MetaData, QString> meta = track.info.metaData();
        settings.setValue(QStringLiteral("artist%1").arg(i), meta[Qmmp::ARTIST]);
        settings.setValue(QStringLiteral("title%1").arg(i), meta[Qmmp::TITLE]);
        settings.setValue(QStringLiteral("genre%1").arg(i), meta[Qmmp::GENRE]);
        settings.setValue(QStringLiteral("album%1").arg(i), meta[Qmmp::ALBUM]);
        settings.setValue(QStringLiteral("year%1").arg(i), meta[Qmmp::YEAR]);
    }
}

bool DecoderCDAudio::readFromCache(QList<CDATrack> *tracks, uint disc_id)
{
    QString path = Qmmp::configDir();
    path += QStringLiteral("/cddbcache/%1").arg(disc_id, 0, 16);
    if(!QFile::exists(path))
        return false;
    QSettings settings(path, QSettings::IniFormat);
    int count = settings.value(u"count"_s).toInt();
    if(count != tracks->count())
        return false;
    for(int i = 0; i < count; ++i)
    {
        (*tracks)[i].info.setValue(Qmmp::ARTIST, settings.value(QStringLiteral("artist%1").arg(i)).toString());
        (*tracks)[i].info.setValue(Qmmp::TITLE, settings.value(QStringLiteral("title%1").arg(i)).toString());
        (*tracks)[i].info.setValue(Qmmp::GENRE, settings.value(QStringLiteral("genre%1").arg(i)).toString());
        (*tracks)[i].info.setValue(Qmmp::ALBUM, settings.value(QStringLiteral("album%1").arg(i)).toString());
        (*tracks)[i].info.setValue(Qmmp::YEAR, settings.value(QStringLiteral("year%1").arg(i)).toString());
    }
    return true;
}

qint64 DecoderCDAudio::calculateTrackLength(lsn_t startlsn, lsn_t endlsn)
{
    return ((endlsn - startlsn + 1) * 1000) / 75;
}

void DecoderCDAudio::clearTrackCache()
{
    m_track_cache.clear();
}

bool DecoderCDAudio::initialize()
{
    m_bitrate = 0;
    m_totalTime = 0;
    //extract track from url
    int track_number = -1;
    QString device_path = TrackInfo::pathFromUrl(m_url, &track_number);

    track_number = qMax(track_number, 1);
    QList<CDATrack> tracks = DecoderCDAudio::generateTrackList(device_path); //generate track list
    if (tracks.isEmpty())
    {
        qCWarning(plugin, "initialize failed");
        return false;
    }
    //find track by number
    int track_at = -1;
    for (int i = 0; i < tracks.size(); ++i)
        if (tracks[i].info.value(Qmmp::TRACK).toInt() == track_number)
        {
            track_at = i;
            break;
        }
    if (track_at < 0)
    {
        qCWarning(plugin, "invalid track number");
        return false;
    }

    if (device_path.isEmpty() || device_path == "/"_L1) //try default path from config
    {
        QSettings settings;
        device_path = settings.value("cdaudio/device"_L1).toString();
        m_url = QStringLiteral("cdda://%1#%2").arg(device_path).arg(track_number);
    }

    if (device_path.isEmpty() || device_path == "/"_L1)
    {
        char **cd_drives = cdio_get_devices_with_cap(nullptr, CDIO_FS_AUDIO, true); //get drive list with CDA disks
        // open first audio capable cd drive
        if (cd_drives && *cd_drives)
        {
            m_cdio = cdio_open_cd(*cd_drives);
            if (!m_cdio)
            {
                qCWarning(plugin, "failed to open CD.");
                cdio_free_device_list(cd_drives);
                return false;
            }
            qCDebug(plugin, "found cd audio capable drive \"%s\"", *cd_drives);
            cdio_free_device_list(cd_drives);  //free device list
        }
        else
        {
            qCWarning(plugin, "unable to find cd audio drive.");
            return false;
        }
    }
    else
    {
        m_cdio = cdio_open_cd(device_path.toLatin1().constData());
        if (!m_cdio)
        {
            qCWarning(plugin, "failed to open CD.");
            return false;
        }
        qCDebug(plugin, "using cd audio capable drive \"%s\"", qPrintable(device_path));
    }
    configure(44100, 2, Qmmp::PCM_S16LE);
    m_bitrate = 1411;
    m_totalTime = tracks[track_at].info.duration();
    m_first_sector = tracks[track_at].first_sector;
    m_current_sector = tracks[track_at].first_sector;
    m_last_sector = tracks[track_at].last_sector;
    addMetaData(tracks[track_at].info.metaData()); //send metadata
    setProperty(Qmmp::FORMAT_NAME, u"CDDA"_s);
    setProperty(Qmmp::BITRATE, m_bitrate);
    qCDebug(plugin, "initialize succes");
    return true;
}


qint64 DecoderCDAudio::totalTime() const
{
    return m_totalTime;
}

int DecoderCDAudio::bitrate() const
{
    return m_bitrate;
}

qint64 DecoderCDAudio::read(unsigned char *audio, qint64 maxSize)
{
    if(!m_buffer_at)
    {

        lsn_t secorts_to_read = qMin(CDDA_SECTORS, (m_last_sector - m_current_sector + 1));

        if (secorts_to_read <= 0)
            return 0;

        if (cdio_read_audio_sectors(m_cdio, m_buffer,
                                    m_current_sector, secorts_to_read) != DRIVER_OP_SUCCESS)
        {
            m_buffer_at = 0;
            return -1;
        }

        m_buffer_at = secorts_to_read * CDIO_CD_FRAMESIZE_RAW;
        m_current_sector += secorts_to_read;
    }

    if(m_buffer_at > 0)
    {
        long len = qMin(maxSize, m_buffer_at);
        memcpy(audio, m_buffer, len);
        m_buffer_at -= len;
        memmove(m_buffer, m_buffer + len, m_buffer_at);
        return len;
    }
    return 0;
}

void DecoderCDAudio::seek(qint64 pos)
{
    m_current_sector = m_first_sector + pos * 75 / 1000;
    m_buffer_at = 0;
}
