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

#include <QFileInfo>
#include <QSettings>
#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/sidbuilder.h>
#include <sidplayfp/SidConfig.h>
#if LIBSIDPLAYFP_VERSION_MAJ >= 3
#include <sidplayfp/builders/sidlite.h>
#else
#include <sidplayfp/builders/resid.h>
#endif
#ifdef HAVE_RESIDFP_HEADER
#include <sidplayfp/builders/residfp.h>
#endif
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/SidDatabase.h>
#include <qmmp/trackinfo.h>
#include "decoder_sid.h"

// Decoder class
DecoderSID::DecoderSID(SidDatabase *db, const QString &url) : Decoder(),
    m_url(url),
    m_player(new sidplayfp()),
    m_db(db),
    m_tune(nullptr)
{}

DecoderSID::~DecoderSID()
{
    delete m_player;
}

bool DecoderSID::initialize()
{
    m_length_in_bytes = 0;
    m_read_bytes = 0;
    int track = -1;
    QString path = TrackInfo::pathFromUrl(m_url, &track);

    m_tune = std::make_unique<SidTune>(nullptr);
    m_tune->load(qPrintable(path));
    if(!m_tune->getInfo())
    {
        qCWarning(plugin, "unable to load tune, error: %s", m_tune->statusString());
        return false;
    }
    int count = m_tune->getInfo()->songs();

    if(track > count || track < 1)
    {
        qCWarning(plugin, "track number is out of range");
        return false;
    }

    m_tune->selectSong(track);

    if(!m_tune->getStatus())
    {
        qCWarning(plugin, "error: %s", m_tune->statusString());
        return false;
    }

    //send metadata for pseudo-protocol
    const SidTuneInfo *tune_info = m_tune->getInfo();
    QMap<Qmmp::MetaData, QString> metadata;
    metadata.insert(Qmmp::TITLE, QString::fromUtf8(tune_info->infoString(0)));
    metadata.insert(Qmmp::ARTIST, QString::fromUtf8(tune_info->infoString(1)));
    metadata.insert(Qmmp::COMMENT, QString::fromUtf8(tune_info->commentString(0)));
    metadata.insert(Qmmp::TRACK, QString::number(track));
    addMetaData(metadata);

    //read settings
    QSettings settings;
    settings.beginGroup(u"SID"_s);
    if(settings.value(u"use_hvsc"_s, false).toBool())
    {
        char md5[SidTune::MD5_LENGTH + 1];
        m_tune->createMD5(md5);
        m_length = m_db->length(md5, track);
    }

    if(m_length <= 0)
        m_length = settings.value("song_length"_L1, 180).toInt();

    qCDebug(plugin, "song length: %d", m_length);

#if LIBSIDPLAYFP_VERSION_MAJ >= 3
#ifdef HAVE_RESIDFP_HEADER
    if(settings.value(u"engine"_s, u"sidlite"_s).toString() == "residfp"_L1)
    {
        m_builder = std::make_unique<ReSIDfpBuilder>("ReSIDfp");
    }
    else
#endif
    {
        m_builder = std::make_unique<SIDLiteBuilder>("SIDLite");
    }
#else
#ifdef HAVE_RESIDFP_HEADER
    if(settings.value(u"engine"_s, u"residfp"_s).toString() == "residfp"_L1)
    {
        m_builder = std::make_unique<ReSIDfpBuilder>("ReSIDfp");
    }
    else
#endif
    {
        m_builder = std::make_unique<ReSIDBuilder>("ReSID");
    }
    m_builder->create(m_player->info().maxsids());
#endif

    qCDebug(plugin, "using %s emulation", m_builder->name());

    SidConfig cfg = m_player->config();
    cfg.frequency = settings.value(u"sample_rate"_s, 48000).toInt();
    int sm = settings.value(u"resampling_method"_s, SidConfig::INTERPOLATE).toInt();
    cfg.samplingMethod = static_cast<SidConfig::sampling_method_t>(sm);

    cfg.sidEmulation = m_builder.get();
#if LIBSIDPLAYFP_VERSION_MAJ < 3
    cfg.playback     = SidConfig::STEREO;
    cfg.fastSampling = settings.value(u"fast_resampling"_s, false).toBool();
#endif
    settings.endGroup();

    if(!m_player->config(cfg))
    {
        qCWarning(plugin, "unable to load config, error: %s", m_player->error());
        return false;
    }

    if(!m_player->load(m_tune.get()))
    {
        qCWarning(plugin, "unable to load tune, error: %s", m_player->error());
        return false;
    }

#if (LIBSIDPLAYFP_VERSION_MAJ > 2) || ((LIBSIDPLAYFP_VERSION_MAJ == 2) && (LIBSIDPLAYFP_VERSION_MIN >= 15))
    m_player->initMixer(true);
#endif

    configure(cfg.frequency, 2);
    m_length_in_bytes = audioParameters().sampleRate() *
            audioParameters().frameSize() * m_length;
    qCDebug(plugin, "initialize succes");
    return true;
}

qint64 DecoderSID::totalTime() const
{
    return 0;
}

void DecoderSID::seek(qint64 pos)
{
    Q_UNUSED(pos);
}

int DecoderSID::bitrate() const
{
    return 8;
}

qint64 DecoderSID::read(unsigned char *data, qint64 size)
{
    size = qMin(size, qMax(m_length_in_bytes - m_read_bytes, qint64(0)));
    size -= size % 4;
    if(size <= 0)
        return 0;

#if (LIBSIDPLAYFP_VERSION_MAJ > 2) || ((LIBSIDPLAYFP_VERSION_MAJ == 2) && (LIBSIDPLAYFP_VERSION_MIN >= 15))
    int cycles = size * 1000000 / m_player->config().frequency; //ms
    int samples = m_player->play(cycles);
    int decoded_bytes = m_player->mix((short *)data, samples) * 2;
#else
    int decoded_bytes = m_player->play((short *)data, size / 2) * 2;
#endif
    m_read_bytes += decoded_bytes;
    return decoded_bytes;
}
