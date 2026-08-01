/***************************************************************************
 *   Copyright (C) 2011-2026 by Ilya Kotov                                 *
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
#include <QFile>
#include <QRegularExpression>
#include <qmmp/cueparser.h>
#include <qmmp/decoderfactory.h>
#include "decoder_ffmpeg.h"
#include "decoder_ffmpegm4b.h"

DecoderFFmpegM4b::DecoderFFmpegM4b(DecoderFactory *factory, const QString &url) : Decoder(),
    m_url(url), m_factory(factory)
{}

DecoderFFmpegM4b::~DecoderFFmpegM4b()
{
    delete m_decoder;
    m_decoder = nullptr;
    delete [] m_buf;
    m_buf = nullptr;
    if(m_input)
        m_input->deleteLater();
    m_input = nullptr;
}

bool DecoderFFmpegM4b::initialize()
{
    if(!m_url.startsWith(u"m4b://"_s))
    {
        qCWarning(plugin, "invalid url.");
        return false;
    }
    QString filePath = TrackInfo::pathFromUrl(m_url, &m_track);

    AVFormatContext *in = nullptr;
#ifdef Q_OS_WIN
    if(avformat_open_input(&in, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
#else
    if(avformat_open_input(&in, filePath.toLocal8Bit().constData(), nullptr, nullptr) < 0)
#endif
    {
        qCDebug(plugin, "unable to open file");
        return false;
    }

    avformat_find_stream_info(in, nullptr);

    if(in->nb_chapters <= 1)
    {
        avformat_close_input(&in);
        qCWarning(plugin, "unable to find chapters");
        return false;
    }

    if(m_track > int(in->nb_chapters) || m_track < 1)
    {
        avformat_close_input(&in);
        qCWarning(plugin, "invalid track number");
        return false;
    }

    QList<TrackInfo> tracks = m_factory->createPlayList(filePath, TrackInfo::AllParts, nullptr);
    if(tracks.isEmpty() || tracks.count() != int(in->nb_chapters))
    {
        avformat_close_input(&in);
        qCWarning(plugin, "unable to find tracks");
        return false;
    }

    for(int i = 0; i < tracks.count(); ++i)
    {
        AVChapter *chapter = in->chapters[i];
        ChapterInfo chapterInfo = {
            .info = tracks[i],
            .offset = chapter->start * chapter->time_base.num * 1000 / chapter->time_base.den,
            .duration = (chapter->end - chapter->start) * chapter->time_base.num * 1000 / chapter->time_base.den,
            .url = QStringLiteral("m4b://%1#%2").arg(filePath).arg(i + 1)
        };

        m_chapters << chapterInfo;
    }

    tracks.clear();
    avformat_close_input(&in);

    m_input = new QFile(filePath);
    if(!m_input->open(QIODevice::ReadOnly))
    {
        qCWarning(plugin, "unable to open file; error: %s", qPrintable(m_input->errorString()));
        return false;
    }

    m_duration = m_chapters[m_track - 1].duration;
    m_offset = m_chapters[m_track - 1].offset;
    m_decoder = new DecoderFFmpeg(filePath, m_input);
    if(!m_decoder->initialize())
    {
        qCWarning(plugin, "DecoderFFapCUE: invalid audio file");
        return false;
    }
    m_decoder->seek(m_offset);

    configure(m_decoder->audioParameters());

    m_trackSize = audioParameters().sampleRate() * audioParameters().channels() *
            audioParameters().sampleSize() * m_duration / 1000;
    m_written = 0;

    m_frameSize = audioParameters().sampleSize() * audioParameters().channels();

    setReplayGainInfo(m_decoder->replayGainInfo()); //send ReplayGaing info
    addMetaData(m_chapters[m_track - 1].info.metaData()); //send metadata
    return true;
}

qint64 DecoderFFmpegM4b::totalTime() const
{
    return m_decoder ? m_duration : 0;
}

void DecoderFFmpegM4b::seek(qint64 pos)
{
    m_decoder->seek(m_offset + pos);
    m_written = audioParameters().sampleRate() *
            audioParameters().channels() *
            audioParameters().sampleSize() * pos/1000;
}

qint64 DecoderFFmpegM4b::read(unsigned char *data, qint64 size)
{
    if(m_trackSize - m_written < m_frameSize) //end of cue track
        return 0;

    qint64 len = 0;

    if(m_buf) //read remaining data first
    {
        len = qMin(m_bufSize, size);
        memmove(data, m_buf, len);
        if(size >= m_bufSize)
        {
            delete[] m_buf;
            m_buf = nullptr;
            m_bufSize = 0;
        }
        else
            memmove(m_buf, m_buf + len, size - len);
    }
    else
        len = m_decoder->read(data, size);

    if(len <= 0) //end of file
        return 0;

    if(len + m_written <= m_trackSize)
    {
        m_written += len;
        return len;
    }

    qint64 len2 = qMax(qint64(0), m_trackSize - m_written);
    len2 = (len2 / m_frameSize) * m_frameSize; //integer number of samples
    m_written += len2;
    //save data of the next track
    delete[] m_buf;
    m_bufSize = len - len2;
    m_buf = new char[m_bufSize];
    memmove(m_buf, data + len2, m_bufSize);
    return len2;
}

int DecoderFFmpegM4b::bitrate() const
{
    return m_decoder->bitrate();
}

QString DecoderFFmpegM4b::nextURL() const
{
    if(m_track + 1 <= m_chapters.count())
        return m_chapters[m_track].url;

    return QString();
}

void DecoderFFmpegM4b::next()
{
    if(m_track + 1 <= m_chapters.count())
    {
        m_track++;
        m_duration = m_chapters[m_track - 1].duration;
        m_offset = m_chapters[m_track - 1].offset;
        m_trackSize = audioParameters().sampleRate() *
                audioParameters().channels() *
                audioParameters().sampleSize() * m_duration/1000;
        addMetaData(m_chapters[m_track - 1].info.metaData());
        setReplayGainInfo(m_decoder->replayGainInfo());
        m_written = 0;
    }
}
